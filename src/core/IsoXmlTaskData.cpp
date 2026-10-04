#include "IsoXmlTaskData.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <locale>
#include <map>
#include <sstream>

namespace agisotc
{
	namespace
	{
		void append_utf8(std::string &out, std::uint32_t codePoint)
		{
			if (codePoint < 0x80)
			{
				out += static_cast<char>(codePoint);
			}
			else if (codePoint < 0x800)
			{
				out += static_cast<char>(0xC0 | (codePoint >> 6));
				out += static_cast<char>(0x80 | (codePoint & 0x3F));
			}
			else if (codePoint < 0x10000)
			{
				out += static_cast<char>(0xE0 | (codePoint >> 12));
				out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
				out += static_cast<char>(0x80 | (codePoint & 0x3F));
			}
			else
			{
				out += static_cast<char>(0xF0 | (codePoint >> 18));
				out += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
				out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
				out += static_cast<char>(0x80 | (codePoint & 0x3F));
			}
		}

		std::string decode_entities(std::string_view text)
		{
			std::string out;
			out.reserve(text.size());
			for (std::size_t i = 0; i < text.size(); ++i)
			{
				if ('&' != text[i])
				{
					out += text[i];
					continue;
				}
				const auto end = text.find(';', i);
				if (std::string_view::npos == end)
				{
					out += text[i];
					continue;
				}
				const auto entity = text.substr(i + 1, end - i - 1);
				if ("lt" == entity) out += '<';
				else if ("gt" == entity) out += '>';
				else if ("amp" == entity) out += '&';
				else if ("quot" == entity) out += '"';
				else if ("apos" == entity) out += '\'';
				else if (!entity.empty() && ('#' == entity[0]))
				{
					const bool hex = (entity.size() > 1) && (('x' == entity[1]) || ('X' == entity[1]));
					const std::string digits(entity.substr(hex ? 2 : 1));
					append_utf8(out, static_cast<std::uint32_t>(std::strtoul(digits.c_str(), nullptr, hex ? 16 : 10)));
				}
				else
				{
					out.append(text.substr(i, end - i + 1)); // unknown entity: kept as it is
				}
				i = end;
			}
			return out;
		}

		bool is_name_char(char character)
		{
			return (0 != std::isalnum(static_cast<unsigned char>(character))) || ('_' == character) || ('-' == character) ||
			  (':' == character) || ('.' == character) || (static_cast<unsigned char>(character) >= 0x80);
		}

		void skip_space(std::string_view text, std::size_t &at)
		{
			while ((at < text.size()) && (0 != std::isspace(static_cast<unsigned char>(text[at])))) ++at;
		}

		std::optional<double> parse_double(const std::string *text)
		{
			if (nullptr == text) return std::nullopt;
			std::istringstream stream(*text);
			stream.imbue(std::locale::classic());
			double value = 0.0;
			stream >> value;
			if (stream.fail()) return std::nullopt;
			return value;
		}

		std::optional<long long> parse_integer(const std::string *text, int base = 10)
		{
			if ((nullptr == text) || text->empty()) return std::nullopt;
			char *end = nullptr;
			const long long value = std::strtoll(text->c_str(), &end, base);
			if (end == text->c_str()) return std::nullopt;
			return value;
		}

		std::string text_of(const std::string *text)
		{
			return (nullptr == text) ? std::string() : *text;
		}

		std::vector<GeoPoint> points_of(const XmlElement &lineString)
		{
			std::vector<GeoPoint> points;
			for (const auto &point : lineString.children)
			{
				if ("PNT" != point.name) continue;
				const auto north = parse_double(point.attribute("C"));
				const auto east = parse_double(point.attribute("D"));
				if (north && east) points.push_back({ *north, *east });
			}
			return points;
		}

		/// The surfaces of a polygon: each exterior ring (LSG type 1) with the holes (type 2) after it.
		std::vector<PrescriptionPolygon> polygons_of(const XmlElement &polygon)
		{
			std::vector<PrescriptionPolygon> surfaces;
			for (const auto &lineString : polygon.children)
			{
				if ("LSG" != lineString.name) continue;
				const auto type = parse_integer(lineString.attribute("A")).value_or(0);
				auto points = points_of(lineString);
				if (points.size() < 3) continue;
				if (1 == type)
				{
					PrescriptionPolygon surface;
					surface.exterior = std::move(points);
					surfaces.push_back(std::move(surface));
				}
				else if ((2 == type) && !surfaces.empty())
				{
					surfaces.back().holes.push_back(std::move(points));
				}
			}
			return surfaces;
		}

		std::optional<std::vector<std::uint8_t>> read_either(const TaskDataFileReader &readFile, const std::string &stem, const char *lower, const char *upper)
		{
			if (!readFile) return std::nullopt;
			auto data = readFile(stem + upper);
			if (!data) data = readFile(stem + lower);
			return data;
		}
	} // namespace

	const std::string *XmlElement::attribute(std::string_view key) const
	{
		for (const auto &[name, value] : attributes)
		{
			if (name == key) return &value;
		}
		return nullptr;
	}

	bool parse_xml(std::string_view text, XmlElement &root, std::string &error)
	{
		if ((text.size() >= 3) && (static_cast<unsigned char>(text[0]) == 0xEF) && (static_cast<unsigned char>(text[1]) == 0xBB) && (static_cast<unsigned char>(text[2]) == 0xBF))
		{
			text.remove_prefix(3);
		}
		auto fail = [&error](const std::string &message) {
			error = message;
			return false;
		};
		root = XmlElement();
		bool haveRoot = false;
		std::vector<XmlElement *> open;
		std::size_t at = 0;
		while (at < text.size())
		{
			const auto tag = text.find('<', at);
			if (std::string_view::npos == tag) break;
			at = tag + 1;
			if (text.substr(at, 3) == "!--")
			{
				const auto end = text.find("-->", at + 3);
				if (std::string_view::npos == end) return fail("Unterminated comment");
				at = end + 3;
				continue;
			}
			if (text.substr(at, 8) == "![CDATA[")
			{
				const auto end = text.find("]]>", at + 8);
				if (std::string_view::npos == end) return fail("Unterminated CDATA section");
				at = end + 3;
				continue;
			}
			if ((at < text.size()) && (('?' == text[at]) || ('!' == text[at])))
			{
				// Declaration, processing instruction or document type: skipped.
				int depth = 0;
				for (; at < text.size(); ++at)
				{
					if ('[' == text[at]) ++depth;
					else if (']' == text[at]) --depth;
					else if (('>' == text[at]) && (depth <= 0)) break;
				}
				++at;
				continue;
			}
			if ((at < text.size()) && ('/' == text[at]))
			{
				++at;
				const auto start = at;
				while ((at < text.size()) && is_name_char(text[at])) ++at;
				const auto name = text.substr(start, at - start);
				skip_space(text, at);
				if ((at >= text.size()) || ('>' != text[at])) return fail("Malformed end tag");
				++at;
				if (open.empty() || (open.back()->name != name)) return fail("Unexpected end tag </" + std::string(name) + ">");
				open.pop_back();
				continue;
			}

			XmlElement element;
			const auto start = at;
			while ((at < text.size()) && is_name_char(text[at])) ++at;
			element.name = std::string(text.substr(start, at - start));
			if (element.name.empty()) return fail("Malformed start tag");
			bool selfClosing = false;
			while (true)
			{
				skip_space(text, at);
				if (at >= text.size()) return fail("Unterminated tag <" + element.name + ">");
				if ('>' == text[at])
				{
					++at;
					break;
				}
				if (('/' == text[at]) && ((at + 1) < text.size()) && ('>' == text[at + 1]))
				{
					at += 2;
					selfClosing = true;
					break;
				}
				const auto nameStart = at;
				while ((at < text.size()) && is_name_char(text[at])) ++at;
				const auto attributeName = text.substr(nameStart, at - nameStart);
				skip_space(text, at);
				if (attributeName.empty() || (at >= text.size()) || ('=' != text[at])) return fail("Malformed attribute in <" + element.name + ">");
				++at;
				skip_space(text, at);
				if ((at >= text.size()) || (('"' != text[at]) && ('\'' != text[at]))) return fail("Unquoted attribute in <" + element.name + ">");
				const char quote = text[at++];
				const auto end = text.find(quote, at);
				if (std::string_view::npos == end) return fail("Unterminated attribute in <" + element.name + ">");
				element.attributes.emplace_back(std::string(attributeName), decode_entities(text.substr(at, end - at)));
				at = end + 1;
			}

			XmlElement *placed = nullptr;
			if (open.empty())
			{
				if (haveRoot) return fail("More than one root element");
				root = std::move(element);
				haveRoot = true;
				placed = &root;
			}
			else
			{
				open.back()->children.push_back(std::move(element));
				placed = &open.back()->children.back();
			}
			if (!selfClosing) open.push_back(placed);
		}
		if (!haveRoot) return fail("No root element");
		if (!open.empty()) return fail("Unclosed element <" + open.back()->name + ">");
		return true;
	}

	bool import_task_data(std::string_view xml, const TaskDataFileReader &readFile, TaskDataImport &result, std::string &error)
	{
		result = TaskDataImport();
		XmlElement root;
		if (!parse_xml(xml, root, error)) return false;
		if ("ISO11783_TaskData" != root.name)
		{
			error = "Not ISO 11783 task data (root element <" + root.name + ">)";
			return false;
		}

		// External files (XFR) hold parts of the transfer set: their elements belong to the root.
		std::vector<XmlElement> external;
		for (const auto &reference : root.children)
		{
			if ("XFR" != reference.name) continue;
			const auto stem = text_of(reference.attribute("A"));
			const auto data = read_either(readFile, stem, ".xml", ".XML");
			XmlElement contents;
			std::string externalError;
			if (!data)
			{
				result.warnings.push_back("External file " + stem + ".XML is missing");
			}
			else if (!parse_xml(std::string_view(reinterpret_cast<const char *>(data->data()), data->size()), contents, externalError))
			{
				result.warnings.push_back("External file " + stem + ".XML: " + externalError);
			}
			else
			{
				for (auto &child : contents.children) external.push_back(std::move(child));
			}
		}
		for (auto &child : external) root.children.push_back(std::move(child));

		std::map<std::string, ValuePresentation> presentations;
		for (const auto &element : root.children)
		{
			if ("VPN" != element.name) continue;
			ValuePresentation presentation;
			presentation.offset = static_cast<std::int32_t>(parse_integer(element.attribute("B")).value_or(0));
			presentation.scale = parse_double(element.attribute("C")).value_or(1.0);
			presentation.decimals = static_cast<int>(parse_integer(element.attribute("D")).value_or(0));
			presentation.unit = text_of(element.attribute("E"));
			presentations[text_of(element.attribute("A"))] = presentation;
		}

		for (const auto &element : root.children)
		{
			if ("PFD" != element.name) continue;
			ImportedField field;
			field.id = text_of(element.attribute("A"));
			field.name = text_of(element.attribute("C"));
			if (field.name.empty()) field.name = field.id;
			for (const auto &polygon : element.children)
			{
				if (("PLN" != polygon.name) || (1 != parse_integer(polygon.attribute("A")).value_or(0))) continue;
				const auto surfaces = polygons_of(polygon);
				if (!surfaces.empty())
				{
					field.boundary = surfaces.front().exterior;
					break;
				}
			}
			result.fields.push_back(std::move(field));
		}

		for (const auto &element : root.children)
		{
			if ("TSK" != element.name) continue;
			ImportedTask task;
			task.id = text_of(element.attribute("A"));
			task.name = text_of(element.attribute("B"));
			if (task.name.empty()) task.name = task.id;
			task.partfieldId = text_of(element.attribute("E"));
			auto &prescription = task.prescription;
			prescription.name = task.name;
			auto zoneCode = [&element](const char *key) -> std::optional<std::uint8_t> {
				const auto value = parse_integer(element.attribute(key));
				if (!value || (*value < 0) || (*value > 254)) return std::nullopt;
				return static_cast<std::uint8_t>(*value);
			};
			prescription.defaultZone = zoneCode("H");
			prescription.positionLostZone = zoneCode("I");
			prescription.outOfFieldZone = zoneCode("J");

			const XmlElement *gridElement = nullptr;
			for (const auto &child : element.children)
			{
				if ("GRD" == child.name)
				{
					gridElement = &child;
					continue;
				}
				if ("TZN" != child.name) continue;
				TreatmentZone zone;
				zone.code = static_cast<std::uint8_t>(std::clamp<long long>(parse_integer(child.attribute("A")).value_or(0), 0, 254));
				zone.name = text_of(child.attribute("B"));
				for (const auto &item : child.children)
				{
					if ("PDV" == item.name)
					{
						const auto ddi = parse_integer(item.attribute("A"), 16);
						if (!ddi || (*ddi < 0) || (*ddi > 0xFFFF))
						{
							result.warnings.push_back("Task " + task.name + ": a variable without a valid DDI is skipped");
							continue;
						}
						PrescriptionValue value;
						value.ddi = static_cast<std::uint16_t>(*ddi);
						value.value = static_cast<std::int32_t>(parse_integer(item.attribute("B")).value_or(0));
						value.productId = text_of(item.attribute("C"));
						value.deviceElementId = text_of(item.attribute("D"));
						const auto presentation = presentations.find(text_of(item.attribute("E")));
						if (presentation != presentations.end()) value.presentation = presentation->second;
						if (const auto practice = parse_integer(item.attribute("F"))) value.culturalPractice = static_cast<std::int32_t>(*practice);
						if (const auto instance = parse_integer(item.attribute("G"))) value.elementTypeInstance = static_cast<std::int32_t>(*instance);
						zone.values.push_back(std::move(value));
					}
					else if ("PLN" == item.name)
					{
						for (auto &surface : polygons_of(item)) zone.polygons.push_back(std::move(surface));
					}
				}
				prescription.zones.push_back(std::move(zone));
			}

			if (nullptr != gridElement)
			{
				PrescriptionGrid grid;
				grid.minimumNorth = parse_double(gridElement->attribute("A")).value_or(0.0);
				grid.minimumEast = parse_double(gridElement->attribute("B")).value_or(0.0);
				grid.cellNorthSize = parse_double(gridElement->attribute("C")).value_or(0.0);
				grid.cellEastSize = parse_double(gridElement->attribute("D")).value_or(0.0);
				grid.columns = static_cast<std::uint32_t>(std::max<long long>(0, parse_integer(gridElement->attribute("E")).value_or(0)));
				grid.rows = static_cast<std::uint32_t>(std::max<long long>(0, parse_integer(gridElement->attribute("F")).value_or(0)));
				grid.type = static_cast<std::uint8_t>(parse_integer(gridElement->attribute("I")).value_or(1));
				grid.templateZoneCode = static_cast<std::uint8_t>(std::clamp<long long>(parse_integer(gridElement->attribute("J")).value_or(0), 0, 254));
				const auto fileName = text_of(gridElement->attribute("G"));
				const auto data = read_either(readFile, fileName, ".bin", ".BIN");
				const std::size_t cells = static_cast<std::size_t>(grid.rows) * grid.columns;
				const auto *templateZone = prescription.zone(grid.templateZoneCode);
				const std::size_t perCell = ((2 == grid.type) && (nullptr != templateZone)) ? templateZone->values.size() : 1;
				const std::size_t expected = (2 == grid.type) ? (cells * perCell * 4) : cells;
				if ((grid.type != 1) && (grid.type != 2))
				{
					result.warnings.push_back("Task " + task.name + ": grid type " + std::to_string(grid.type) + " is unknown, the grid is skipped");
				}
				else if ((2 == grid.type) && (nullptr == templateZone))
				{
					result.warnings.push_back("Task " + task.name + ": the grid's treatment zone " + std::to_string(grid.templateZoneCode) + " is missing, the grid is skipped");
				}
				else if (!data)
				{
					result.warnings.push_back("Task " + task.name + ": grid file " + fileName + ".bin is missing");
				}
				else
				{
					if (data->size() != expected)
					{
						result.warnings.push_back("Task " + task.name + ": grid file " + fileName + ".bin has " + std::to_string(data->size()) +
						                          " bytes, " + std::to_string(expected) + " expected");
					}
					if (1 == grid.type)
					{
						grid.zoneCodes.assign(cells, 0xFF);
						std::copy_n(data->begin(), std::min(cells, data->size()), grid.zoneCodes.begin());
					}
					else
					{
						// Signed 32-bit values, least significant byte first (ISO 11783-6 byte order).
						grid.values.assign(cells * perCell, 0);
						for (std::size_t i = 0; (i < grid.values.size()) && (((i * 4) + 3) < data->size()); ++i)
						{
							const std::uint8_t *bytes = data->data() + (i * 4);
							grid.values[i] = static_cast<std::int32_t>(static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1]) << 8) |
							                                           (static_cast<std::uint32_t>(bytes[2]) << 16) | (static_cast<std::uint32_t>(bytes[3]) << 24));
						}
					}
					prescription.grid = std::move(grid);
				}
			}
			prescription.update_layers();
			result.tasks.push_back(std::move(task));
		}
		return true;
	}
} // namespace agisotc
