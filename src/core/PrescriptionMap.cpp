#include "PrescriptionMap.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>

namespace agisotc
{
	namespace
	{
		constexpr double EARTH_RADIUS_M = 6371000.0;
		constexpr double DEGREES_TO_RADIANS = 3.14159265358979323846 / 180.0;

		bool ring_contains(const std::vector<GeoPoint> &ring, GeoPoint point)
		{
			bool inside = false;
			const std::size_t count = ring.size();
			for (std::size_t i = 0, j = count - 1; i < count; j = i++)
			{
				const GeoPoint &a = ring[i];
				const GeoPoint &b = ring[j];
				if (((a.latitude > point.latitude) != (b.latitude > point.latitude)) &&
				    (point.longitude < (b.longitude - a.longitude) * (point.latitude - a.latitude) / (b.latitude - a.latitude) + a.longitude))
				{
					inside = !inside;
				}
			}
			return inside;
		}

		bool near(double left, double right)
		{
			return std::abs(left - right) <= 1e-12;
		}
	} // namespace

	double metres_per_degree_latitude()
	{
		return EARTH_RADIUS_M * DEGREES_TO_RADIANS;
	}

	double metres_per_degree_longitude(double latitude)
	{
		return EARTH_RADIUS_M * DEGREES_TO_RADIANS * std::max(0.01, std::cos(latitude * DEGREES_TO_RADIANS));
	}

	bool polygon_contains(const PrescriptionPolygon &polygon, GeoPoint point)
	{
		if ((polygon.exterior.size() < 3) || !ring_contains(polygon.exterior, point)) return false;
		return std::none_of(polygon.holes.cbegin(), polygon.holes.cend(), [point](const std::vector<GeoPoint> &hole) {
			return (hole.size() >= 3) && ring_contains(hole, point);
		});
	}

	bool PrescriptionLayer::matches(const PrescriptionValue &value) const
	{
		return (value.ddi == ddi) && (value.productId == productId) && (value.deviceElementId == deviceElementId) &&
		  (value.culturalPractice == culturalPractice) && (value.elementTypeInstance == elementTypeInstance);
	}

	std::string PrescriptionLayer::describe() const
	{
		std::ostringstream text;
		text << "DDI " << ddi;
		if (!productId.empty()) text << ", " << productId;
		if (culturalPractice) text << ", practice " << *culturalPractice;
		if (elementTypeInstance) text << ", instance " << *elementTypeInstance;
		if (!deviceElementId.empty()) text << ", " << deviceElementId;
		return text.str();
	}

	void Prescription::update_layers()
	{
		layerList.clear();
		const std::uint8_t templateCode = (grid && (2 == grid->type)) ? grid->templateZoneCode : 0;
		const bool hasTemplate = grid && (2 == grid->type);
		auto include = [this](std::size_t layer, std::int32_t value) {
			auto &entry = layerList[layer];
			entry.minimum = entry.hasValues ? std::min(entry.minimum, value) : value;
			entry.maximum = entry.hasValues ? std::max(entry.maximum, value) : value;
			entry.hasValues = true;
		};
		for (const auto &zone : zones)
		{
			const bool isTemplate = hasTemplate && (zone.code == templateCode);
			// The out-of-field and position-lost values are not on the map: they would only
			// stretch its colour scale (often to 0).
			const bool offMap = zone.polygons.empty() && (zone.code != defaultZone) &&
			  ((zone.code == outOfFieldZone) || (zone.code == positionLostZone));
			for (const auto &value : zone.values)
			{
				auto found = std::find_if(layerList.begin(), layerList.end(), [&value](const PrescriptionLayer &layer) { return layer.matches(value); });
				if (found == layerList.end())
				{
					PrescriptionLayer layer;
					layer.ddi = value.ddi;
					layer.productId = value.productId;
					layer.deviceElementId = value.deviceElementId;
					layer.culturalPractice = value.culturalPractice;
					layer.elementTypeInstance = value.elementTypeInstance;
					layerList.push_back(layer);
					found = layerList.end() - 1;
				}
				if (!found->presentation && value.presentation) found->presentation = value.presentation;
				// The values of a type 2 grid's template zone only stand for the cells' values.
				if (!isTemplate && !offMap) include(static_cast<std::size_t>(found - layerList.begin()), value.value);
			}
		}
		if (hasTemplate)
		{
			if (const auto *templateZone = zone(templateCode))
			{
				const std::size_t perCell = templateZone->values.size();
				for (std::size_t index = 0; index < perCell; ++index)
				{
					const auto found = std::find_if(layerList.begin(), layerList.end(), [&](const PrescriptionLayer &layer) {
						return layer.matches(templateZone->values[index]);
					});
					if (found == layerList.end()) continue;
					for (std::size_t cell = index; cell < grid->values.size(); cell += perCell)
					{
						include(static_cast<std::size_t>(found - layerList.begin()), grid->values[cell]);
					}
				}
			}
		}
	}

	const std::vector<PrescriptionLayer> &Prescription::layers() const
	{
		return layerList;
	}

	bool Prescription::empty() const
	{
		return layerList.empty();
	}

	const TreatmentZone *Prescription::zone(std::uint8_t code) const
	{
		const auto found = std::find_if(zones.cbegin(), zones.cend(), [code](const TreatmentZone &zone) { return zone.code == code; });
		return (found == zones.cend()) ? nullptr : &*found;
	}

	std::optional<std::int32_t> Prescription::zone_value(std::uint8_t code, std::size_t layer) const
	{
		const auto *found = zone(code);
		if ((nullptr == found) || (layer >= layerList.size())) return std::nullopt;
		for (const auto &value : found->values)
		{
			if (layerList[layer].matches(value)) return value.value;
		}
		return std::nullopt;
	}

	std::optional<std::int32_t> Prescription::grid_value(std::size_t layer, GeoPoint position) const
	{
		if (!grid || (layer >= layerList.size()) || (grid->cellNorthSize <= 0.0) || (grid->cellEastSize <= 0.0)) return std::nullopt;
		const double row = std::floor((position.latitude - grid->minimumNorth) / grid->cellNorthSize);
		const double column = std::floor((position.longitude - grid->minimumEast) / grid->cellEastSize);
		if ((row < 0.0) || (column < 0.0) || (row >= grid->rows) || (column >= grid->columns)) return std::nullopt;
		const std::size_t cell = (static_cast<std::size_t>(row) * grid->columns) + static_cast<std::size_t>(column);
		if (1 == grid->type)
		{
			if (cell >= grid->zoneCodes.size()) return std::nullopt;
			return zone_value(grid->zoneCodes[cell], layer);
		}
		const auto *templateZone = zone(grid->templateZoneCode);
		if (nullptr == templateZone) return std::nullopt;
		const std::size_t perCell = templateZone->values.size();
		for (std::size_t index = 0; index < perCell; ++index)
		{
			if (!layerList[layer].matches(templateZone->values[index])) continue;
			const std::size_t at = (cell * perCell) + index;
			if (at < grid->values.size()) return grid->values[at];
		}
		return std::nullopt;
	}

	PrescriptionLookup Prescription::value_at(std::size_t layer, GeoPoint position, bool insideField) const
	{
		PrescriptionLookup lookup;
		if (layer >= layerList.size()) return lookup;
		if (!insideField && outOfFieldZone)
		{
			lookup.value = zone_value(*outOfFieldZone, layer);
			if (lookup.value)
			{
				lookup.source = PrescriptionSource::OutOfField;
				lookup.zoneCode = *outOfFieldZone;
				return lookup;
			}
		}
		for (const auto &zone : zones)
		{
			if (zone.polygons.empty()) continue;
			const bool inside = std::any_of(zone.polygons.cbegin(), zone.polygons.cend(), [position](const PrescriptionPolygon &polygon) {
				return polygon_contains(polygon, position);
			});
			if (!inside) continue;
			lookup.value = zone_value(zone.code, layer);
			if (lookup.value)
			{
				lookup.source = PrescriptionSource::Zone;
				lookup.zoneCode = zone.code;
				return lookup;
			}
		}
		lookup.value = grid_value(layer, position);
		if (lookup.value)
		{
			lookup.source = PrescriptionSource::Grid;
			return lookup;
		}
		if (defaultZone)
		{
			lookup.value = zone_value(*defaultZone, layer);
			if (lookup.value)
			{
				lookup.source = PrescriptionSource::Default;
				lookup.zoneCode = *defaultZone;
			}
		}
		return lookup;
	}

	PrescriptionLookup Prescription::value_without_position(std::size_t layer) const
	{
		PrescriptionLookup lookup;
		if (positionLostZone)
		{
			lookup.value = zone_value(*positionLostZone, layer);
			if (lookup.value)
			{
				lookup.source = PrescriptionSource::PositionLost;
				lookup.zoneCode = *positionLostZone;
			}
		}
		return lookup;
	}

	bool Prescription::bounds(GeoPoint &southWest, GeoPoint &northEast) const
	{
		bool any = false;
		auto include = [&](GeoPoint point) {
			if (!any)
			{
				southWest = northEast = point;
				any = true;
				return;
			}
			southWest.latitude = std::min(southWest.latitude, point.latitude);
			southWest.longitude = std::min(southWest.longitude, point.longitude);
			northEast.latitude = std::max(northEast.latitude, point.latitude);
			northEast.longitude = std::max(northEast.longitude, point.longitude);
		};
		for (const auto &zone : zones)
		{
			for (const auto &polygon : zone.polygons)
			{
				for (const auto &point : polygon.exterior) include(point);
			}
		}
		if (grid && (grid->rows > 0) && (grid->columns > 0))
		{
			include({ grid->minimumNorth, grid->minimumEast });
			include({ grid->minimumNorth + (grid->cellNorthSize * grid->rows), grid->minimumEast + (grid->cellEastSize * grid->columns) });
		}
		return any;
	}

	std::string Prescription::describe() const
	{
		std::ostringstream text;
		text.setf(std::ios::fixed);
		text.precision(1);
		const auto polygonZones = std::count_if(zones.cbegin(), zones.cend(), [](const TreatmentZone &zone) { return !zone.polygons.empty(); });
		bool first = true;
		auto separate = [&]() {
			if (!first) text << ", ";
			first = false;
		};
		if (grid)
		{
			separate();
			text << "Grid type " << static_cast<int>(grid->type) << ", " << grid->columns << " x " << grid->rows << " cells of "
			     << (grid->cellEastSize * metres_per_degree_longitude(grid->minimumNorth)) << " x "
			     << (grid->cellNorthSize * metres_per_degree_latitude()) << " m";
		}
		if (polygonZones > 0)
		{
			separate();
			text << polygonZones << " polygon zone" << ((1 == polygonZones) ? "" : "s");
		}
		separate();
		text << layerList.size() << " layer" << ((1 == layerList.size()) ? "" : "s");
		if (defaultZone) text << ", default zone " << static_cast<int>(*defaultZone);
		if (outOfFieldZone) text << ", out-of-field zone " << static_cast<int>(*outOfFieldZone);
		if (positionLostZone) text << ", position-lost zone " << static_cast<int>(*positionLostZone);
		return text.str();
	}

	void add_test_layer(Prescription &prescription,
	                    GeoPoint southWest,
	                    GeoPoint northEast,
	                    double cellSizeM,
	                    const PrescriptionValue &layer,
	                    TestPattern pattern,
	                    std::int32_t rateA,
	                    std::int32_t rateB,
	                    double patternSizeM)
	{
		cellSizeM = std::clamp(cellSizeM, 0.5, 1000.0);
		patternSizeM = std::max(patternSizeM, cellSizeM);
		const double cellNorth = cellSizeM / metres_per_degree_latitude();
		const double cellEast = cellSizeM / metres_per_degree_longitude((southWest.latitude + northEast.latitude) / 2.0);
		// A whole number of cells stays whole despite rounding in the degrees.
		const auto rows = static_cast<std::uint32_t>(std::clamp(std::ceil(((northEast.latitude - southWest.latitude) / cellNorth) - 1e-6), 1.0, 4000.0));
		const auto columns = static_cast<std::uint32_t>(std::clamp(std::ceil(((northEast.longitude - southWest.longitude) / cellEast) - 1e-6), 1.0, 4000.0));

		auto &grid = prescription.grid;
		const bool sameGrid = grid && (2 == grid->type) && (grid->rows == rows) && (grid->columns == columns) &&
		  near(grid->minimumNorth, southWest.latitude) && near(grid->minimumEast, southWest.longitude) &&
		  near(grid->cellNorthSize, cellNorth) && near(grid->cellEastSize, cellEast) && (nullptr != prescription.zone(grid->templateZoneCode));
		if (!sameGrid)
		{
			std::set<std::uint8_t> used;
			for (const auto &zone : prescription.zones) used.insert(zone.code);
			std::uint8_t templateCode = 1;
			if (grid && (2 == grid->type) && (used.count(grid->templateZoneCode) > 0))
			{
				// The old grid's template is reused, without its variables.
				templateCode = grid->templateZoneCode;
				auto found = std::find_if(prescription.zones.begin(), prescription.zones.end(), [templateCode](const TreatmentZone &zone) { return zone.code == templateCode; });
				found->values.clear();
			}
			else
			{
				while ((used.count(templateCode) > 0) && (templateCode < 254)) ++templateCode;
				TreatmentZone zone;
				zone.code = templateCode;
				zone.name = "Grid values";
				prescription.zones.push_back(zone);
			}
			PrescriptionGrid fresh;
			fresh.minimumNorth = southWest.latitude;
			fresh.minimumEast = southWest.longitude;
			fresh.cellNorthSize = cellNorth;
			fresh.cellEastSize = cellEast;
			fresh.rows = rows;
			fresh.columns = columns;
			fresh.type = 2;
			fresh.templateZoneCode = templateCode;
			grid = fresh;
		}

		auto templateZone = std::find_if(prescription.zones.begin(), prescription.zones.end(), [&](const TreatmentZone &zone) { return zone.code == grid->templateZoneCode; });
		PrescriptionLayer key;
		key.ddi = layer.ddi;
		key.productId = layer.productId;
		key.deviceElementId = layer.deviceElementId;
		key.culturalPractice = layer.culturalPractice;
		key.elementTypeInstance = layer.elementTypeInstance;
		const std::size_t cells = static_cast<std::size_t>(rows) * columns;
		const std::size_t oldPerCell = templateZone->values.size();
		auto index = static_cast<std::size_t>(std::find_if(templateZone->values.begin(), templateZone->values.end(), [&key](const PrescriptionValue &value) { return key.matches(value); }) - templateZone->values.begin());
		if (grid->values.size() != (cells * oldPerCell)) grid->values.assign(cells * oldPerCell, 0);
		if (index == oldPerCell)
		{
			// A new variable: every cell's record gets one more value.
			PrescriptionValue added = layer;
			added.value = 0;
			templateZone->values.push_back(added);
			std::vector<std::int32_t> widened(cells * (oldPerCell + 1), 0);
			for (std::size_t cell = 0; cell < cells; ++cell)
			{
				std::copy_n(grid->values.begin() + static_cast<std::ptrdiff_t>(cell * oldPerCell), oldPerCell, widened.begin() + static_cast<std::ptrdiff_t>(cell * (oldPerCell + 1)));
			}
			grid->values = std::move(widened);
		}
		const std::size_t perCell = templateZone->values.size();
		const double widthM = std::max(cellSizeM, columns * cellSizeM);
		for (std::uint32_t row = 0; row < rows; ++row)
		{
			for (std::uint32_t column = 0; column < columns; ++column)
			{
				const double east = (column + 0.5) * cellSizeM;
				const double north = (row + 0.5) * cellSizeM;
				const auto eastBlock = static_cast<long long>(std::floor(east / patternSizeM));
				const auto northBlock = static_cast<long long>(std::floor(north / patternSizeM));
				std::int32_t value = rateA;
				switch (pattern)
				{
					case TestPattern::Checkerboard: value = (0 == ((eastBlock + northBlock) % 2)) ? rateA : rateB; break;
					case TestPattern::Stripes: value = (0 == (eastBlock % 2)) ? rateA : rateB; break;
					case TestPattern::Bands: value = (0 == (northBlock % 2)) ? rateA : rateB; break;
					case TestPattern::Gradient:
						value = static_cast<std::int32_t>(std::lround(rateA + ((static_cast<double>(rateB) - rateA) * std::clamp(east / widthM, 0.0, 1.0))));
						break;
				}
				grid->values[((static_cast<std::size_t>(row) * columns + column) * perCell) + index] = value;
			}
		}
		prescription.update_layers();
	}

	std::uint8_t add_polygon_zone(Prescription &prescription,
	                              const std::vector<GeoPoint> &ring,
	                              const PrescriptionValue &value,
	                              const std::string &name)
	{
		std::set<std::uint8_t> used;
		for (const auto &zone : prescription.zones) used.insert(zone.code);
		std::uint8_t code = 1;
		while ((used.count(code) > 0) && (code < 254)) ++code;
		TreatmentZone zone;
		zone.code = code;
		zone.name = name;
		zone.values.push_back(value);
		PrescriptionPolygon polygon;
		polygon.exterior = ring;
		zone.polygons.push_back(polygon);
		// Earlier zones win where zones overlap, so a zone drawn later goes in front.
		prescription.zones.insert(prescription.zones.begin(), zone);
		prescription.update_layers();
		return code;
	}
} // namespace agisotc
