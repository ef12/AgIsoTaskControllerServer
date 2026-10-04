#include "PrescriptionJson.hpp"

#include <algorithm>

#include <QByteArray>
#include <QJsonArray>

namespace agisotc
{
	namespace
	{
		QJsonArray encode_ring(const std::vector<GeoPoint> &ring)
		{
			QJsonArray points;
			for (const auto &point : ring) points.push_back(QJsonArray{ point.latitude, point.longitude });
			return points;
		}

		std::vector<GeoPoint> decode_ring(const QJsonArray &points)
		{
			std::vector<GeoPoint> ring;
			for (const auto &entry : points)
			{
				const auto pair = entry.toArray();
				if (pair.size() == 2) ring.push_back({ pair.at(0).toDouble(), pair.at(1).toDouble() });
			}
			return ring;
		}

		void put_zone_code(QJsonObject &object, const char *key, const std::optional<std::uint8_t> &code)
		{
			if (code) object[key] = static_cast<int>(*code);
		}

		std::optional<std::uint8_t> zone_code(const QJsonObject &object, const char *key)
		{
			if (!object.contains(key)) return std::nullopt;
			return static_cast<std::uint8_t>(std::clamp(object.value(key).toInt(), 0, 254));
		}
	} // namespace

	QJsonObject prescription_to_json(const Prescription &prescription)
	{
		QJsonObject object;
		object["name"] = QString::fromStdString(prescription.name);
		put_zone_code(object, "defaultZone", prescription.defaultZone);
		put_zone_code(object, "positionLostZone", prescription.positionLostZone);
		put_zone_code(object, "outOfFieldZone", prescription.outOfFieldZone);
		QJsonArray zones;
		for (const auto &zone : prescription.zones)
		{
			QJsonObject zoneObject;
			zoneObject["code"] = static_cast<int>(zone.code);
			zoneObject["name"] = QString::fromStdString(zone.name);
			QJsonArray values;
			for (const auto &value : zone.values)
			{
				QJsonObject valueObject;
				valueObject["ddi"] = static_cast<int>(value.ddi);
				valueObject["value"] = static_cast<qint64>(value.value);
				if (!value.productId.empty()) valueObject["productId"] = QString::fromStdString(value.productId);
				if (!value.deviceElementId.empty()) valueObject["deviceElementId"] = QString::fromStdString(value.deviceElementId);
				if (value.culturalPractice) valueObject["culturalPractice"] = static_cast<qint64>(*value.culturalPractice);
				if (value.elementTypeInstance) valueObject["elementTypeInstance"] = static_cast<qint64>(*value.elementTypeInstance);
				if (value.presentation)
				{
					valueObject["presentation"] = QJsonObject{ { "offset", static_cast<qint64>(value.presentation->offset) },
						                                       { "scale", value.presentation->scale },
						                                       { "decimals", value.presentation->decimals },
						                                       { "unit", QString::fromStdString(value.presentation->unit) } };
				}
				values.push_back(valueObject);
			}
			zoneObject["values"] = values;
			QJsonArray polygons;
			for (const auto &polygon : zone.polygons)
			{
				QJsonArray holes;
				for (const auto &hole : polygon.holes) holes.push_back(encode_ring(hole));
				polygons.push_back(QJsonObject{ { "exterior", encode_ring(polygon.exterior) }, { "holes", holes } });
			}
			if (!polygons.isEmpty()) zoneObject["polygons"] = polygons;
			zones.push_back(zoneObject);
		}
		object["zones"] = zones;
		if (prescription.grid)
		{
			const auto &grid = *prescription.grid;
			QJsonObject gridObject;
			gridObject["minimumNorth"] = grid.minimumNorth;
			gridObject["minimumEast"] = grid.minimumEast;
			gridObject["cellNorthSize"] = grid.cellNorthSize;
			gridObject["cellEastSize"] = grid.cellEastSize;
			gridObject["columns"] = static_cast<qint64>(grid.columns);
			gridObject["rows"] = static_cast<qint64>(grid.rows);
			gridObject["type"] = static_cast<int>(grid.type);
			gridObject["templateZoneCode"] = static_cast<int>(grid.templateZoneCode);
			if (1 == grid.type)
			{
				const QByteArray codes(reinterpret_cast<const char *>(grid.zoneCodes.data()), static_cast<qsizetype>(grid.zoneCodes.size()));
				gridObject["zoneCodes"] = QString::fromLatin1(codes.toBase64());
			}
			else
			{
				QByteArray bytes;
				bytes.reserve(static_cast<qsizetype>(grid.values.size() * 4));
				for (const auto value : grid.values)
				{
					const auto raw = static_cast<std::uint32_t>(value);
					for (int shift = 0; shift < 32; shift += 8) bytes.push_back(static_cast<char>((raw >> shift) & 0xFF));
				}
				gridObject["values"] = QString::fromLatin1(bytes.toBase64());
			}
			object["grid"] = gridObject;
		}
		return object;
	}

	Prescription prescription_from_json(const QJsonObject &object)
	{
		Prescription prescription;
		prescription.name = object.value("name").toString().toStdString();
		prescription.defaultZone = zone_code(object, "defaultZone");
		prescription.positionLostZone = zone_code(object, "positionLostZone");
		prescription.outOfFieldZone = zone_code(object, "outOfFieldZone");
		for (const auto &zoneEntry : object.value("zones").toArray())
		{
			const auto zoneObject = zoneEntry.toObject();
			TreatmentZone zone;
			zone.code = static_cast<std::uint8_t>(std::clamp(zoneObject.value("code").toInt(), 0, 254));
			zone.name = zoneObject.value("name").toString().toStdString();
			for (const auto &valueEntry : zoneObject.value("values").toArray())
			{
				const auto valueObject = valueEntry.toObject();
				PrescriptionValue value;
				value.ddi = static_cast<std::uint16_t>(valueObject.value("ddi").toInt());
				value.value = static_cast<std::int32_t>(valueObject.value("value").toVariant().toLongLong());
				value.productId = valueObject.value("productId").toString().toStdString();
				value.deviceElementId = valueObject.value("deviceElementId").toString().toStdString();
				if (valueObject.contains("culturalPractice")) value.culturalPractice = static_cast<std::int32_t>(valueObject.value("culturalPractice").toVariant().toLongLong());
				if (valueObject.contains("elementTypeInstance")) value.elementTypeInstance = static_cast<std::int32_t>(valueObject.value("elementTypeInstance").toVariant().toLongLong());
				if (valueObject.contains("presentation"))
				{
					const auto presentationObject = valueObject.value("presentation").toObject();
					ValuePresentation presentation;
					presentation.offset = static_cast<std::int32_t>(presentationObject.value("offset").toVariant().toLongLong());
					presentation.scale = presentationObject.value("scale").toDouble(1.0);
					presentation.decimals = presentationObject.value("decimals").toInt();
					presentation.unit = presentationObject.value("unit").toString().toStdString();
					value.presentation = presentation;
				}
				zone.values.push_back(value);
			}
			for (const auto &polygonEntry : zoneObject.value("polygons").toArray())
			{
				const auto polygonObject = polygonEntry.toObject();
				PrescriptionPolygon polygon;
				polygon.exterior = decode_ring(polygonObject.value("exterior").toArray());
				for (const auto &hole : polygonObject.value("holes").toArray()) polygon.holes.push_back(decode_ring(hole.toArray()));
				zone.polygons.push_back(polygon);
			}
			prescription.zones.push_back(zone);
		}
		if (object.contains("grid"))
		{
			const auto gridObject = object.value("grid").toObject();
			PrescriptionGrid grid;
			grid.minimumNorth = gridObject.value("minimumNorth").toDouble();
			grid.minimumEast = gridObject.value("minimumEast").toDouble();
			grid.cellNorthSize = gridObject.value("cellNorthSize").toDouble();
			grid.cellEastSize = gridObject.value("cellEastSize").toDouble();
			grid.columns = static_cast<std::uint32_t>(gridObject.value("columns").toVariant().toLongLong());
			grid.rows = static_cast<std::uint32_t>(gridObject.value("rows").toVariant().toLongLong());
			grid.type = static_cast<std::uint8_t>(gridObject.value("type").toInt(1));
			grid.templateZoneCode = static_cast<std::uint8_t>(gridObject.value("templateZoneCode").toInt());
			if (1 == grid.type)
			{
				const auto codes = QByteArray::fromBase64(gridObject.value("zoneCodes").toString().toLatin1());
				grid.zoneCodes.assign(codes.cbegin(), codes.cend());
			}
			else
			{
				const auto bytes = QByteArray::fromBase64(gridObject.value("values").toString().toLatin1());
				for (qsizetype at = 0; (at + 3) < bytes.size(); at += 4)
				{
					const auto *raw = reinterpret_cast<const unsigned char *>(bytes.constData() + at);
					grid.values.push_back(static_cast<std::int32_t>(static_cast<std::uint32_t>(raw[0]) | (static_cast<std::uint32_t>(raw[1]) << 8) |
					                                                (static_cast<std::uint32_t>(raw[2]) << 16) | (static_cast<std::uint32_t>(raw[3]) << 24)));
				}
			}
			prescription.grid = grid;
		}
		prescription.update_layers();
		return prescription;
	}
} // namespace agisotc
