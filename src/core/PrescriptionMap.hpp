//================================================================================================
/// @file PrescriptionMap.hpp
///
/// @brief The site-specific part of a task (ISO 11783-10 6.8.1, TC-GEO): treatment zones with
/// their process data variables, placed on the field by polygons or by a grid, and the
/// default, position-lost and out-of-field treatment zones. Answers which setpoint value
/// applies at a position, per layer. No Qt or bus dependency.
//================================================================================================
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace agisotc
{
	/// @brief A WGS84 position in degrees.
	struct GeoPoint
	{
		double latitude = 0.0;
		double longitude = 0.0;
	};

	/// @brief How a value is shown to the operator (ISO 11783-10 D.51 ValuePresentation):
	/// shown = (raw + offset) * scale.
	struct ValuePresentation
	{
		std::int32_t offset = 0;
		double scale = 1.0;
		int decimals = 0;
		std::string unit;
	};

	/// @brief A process data variable of a treatment zone (ISO 11783-10 D.41): the setpoint value
	/// of one DDI, and what it is planned for.
	struct PrescriptionValue
	{
		std::uint16_t ddi = 0;
		std::int32_t value = 0;
		std::string productId; ///< ProductIdRef (C).
		std::string deviceElementId; ///< DeviceElementIdRef (D): an element of the task data's device, not of a client.
		std::optional<std::int32_t> culturalPractice; ///< ActualCulturalPracticeValue (F).
		std::optional<std::int32_t> elementTypeInstance; ///< ElementTypeInstanceValue (G).
		std::optional<ValuePresentation> presentation;
	};

	/// @brief One surface: an exterior ring and its holes.
	struct PrescriptionPolygon
	{
		std::vector<GeoPoint> exterior;
		std::vector<std::vector<GeoPoint>> holes;
	};

	/// @brief An area treated with the same values (ISO 11783-10 D.50).
	struct TreatmentZone
	{
		std::uint8_t code = 0;
		std::string name;
		std::vector<PrescriptionValue> values;
		std::vector<PrescriptionPolygon> polygons; ///< Empty for zones referenced by the grid or the task.
	};

	/// @brief The grid of a task (ISO 11783-10 D.27, 8.6.2). Cells run east along a row, rows run
	/// north, starting at the minimum north and east position.
	struct PrescriptionGrid
	{
		double minimumNorth = 0.0; ///< Degrees.
		double minimumEast = 0.0; ///< Degrees.
		double cellNorthSize = 0.0; ///< Degrees.
		double cellEastSize = 0.0; ///< Degrees.
		std::uint32_t columns = 0;
		std::uint32_t rows = 0;
		std::uint8_t type = 1; ///< 1: a treatment zone code per cell; 2: the values per cell.
		std::uint8_t templateZoneCode = 0; ///< Type 2: the zone whose variables give the order of a cell's values.
		std::vector<std::uint8_t> zoneCodes; ///< Type 1: rows * columns.
		std::vector<std::int32_t> values; ///< Type 2: rows * columns * the template zone's variable count.
	};

	/// @brief The values of one DDI for one product, operation or rate controller instance,
	/// across all treatment zones. What a TC assigns to a setpoint of a client.
	struct PrescriptionLayer
	{
		std::uint16_t ddi = 0;
		std::string productId;
		std::string deviceElementId;
		std::optional<std::int32_t> culturalPractice;
		std::optional<std::int32_t> elementTypeInstance;
		std::optional<ValuePresentation> presentation;
		std::int32_t minimum = 0; ///< Smallest value on the map (out-of-field and position-lost values aside), for a colour scale.
		std::int32_t maximum = 0;
		bool hasValues = false;

		/// @brief True when a variable belongs to this layer.
		bool matches(const PrescriptionValue &value) const;
		/// @brief "DDI 6", with the product, practice and instance when given.
		std::string describe() const;
	};

	/// @brief Where a looked-up value came from.
	enum class PrescriptionSource
	{
		None, ///< No value: the client keeps its setpoint.
		Zone, ///< A polygon treatment zone.
		Grid, ///< A grid cell.
		Default, ///< The task's default treatment zone.
		OutOfField, ///< The out-of-field treatment zone.
		PositionLost ///< The position-lost treatment zone.
	};

	struct PrescriptionLookup
	{
		std::optional<std::int32_t> value;
		PrescriptionSource source = PrescriptionSource::None;
		std::uint8_t zoneCode = 0; ///< The zone, for Zone, Default, OutOfField and PositionLost.
	};

	/// @brief The site-specific prescription of one task.
	class Prescription
	{
	public:
		std::string name;
		std::vector<TreatmentZone> zones;
		std::optional<PrescriptionGrid> grid;
		std::optional<std::uint8_t> defaultZone; ///< DefaultTreatmentZoneCode (H).
		std::optional<std::uint8_t> positionLostZone; ///< PositionLostTreatmentZoneCode (I).
		std::optional<std::uint8_t> outOfFieldZone; ///< OutOfFieldTreatmentZoneCode (J).

		/// @brief Lists the layers again; call after changing zones or the grid.
		void update_layers();
		const std::vector<PrescriptionLayer> &layers() const;
		/// @brief True when there is nothing to apply.
		bool empty() const;

		/// @brief The value of a layer for a device element at a position: the polygon zone it is
		/// in, else the grid cell, else the default zone. Outside the field the out-of-field
		/// zone applies when the task has one.
		/// @param[in] insideField False when the position is outside the field boundary.
		PrescriptionLookup value_at(std::size_t layer, GeoPoint position, bool insideField) const;
		/// @brief The value of a layer while there is no position: the position-lost zone.
		PrescriptionLookup value_without_position(std::size_t layer) const;

		/// @brief The area the zones and the grid cover.
		bool bounds(GeoPoint &southWest, GeoPoint &northEast) const;
		/// @brief One line for the operator: what the map is made of.
		std::string describe() const;

		const TreatmentZone *zone(std::uint8_t code) const;
		/// @brief The value of a layer in a treatment zone.
		std::optional<std::int32_t> zone_value(std::uint8_t code, std::size_t layer) const;
		/// @brief The value of a layer in the grid cell at a position.
		std::optional<std::int32_t> grid_value(std::size_t layer, GeoPoint position) const;

	private:
		std::vector<PrescriptionLayer> layerList;
	};

	/// @brief True when a point lies inside a polygon: inside its exterior ring and in none of its holes.
	bool polygon_contains(const PrescriptionPolygon &polygon, GeoPoint point);

	/// @brief Patterns of a generated test map.
	enum class TestPattern
	{
		Checkerboard, ///< Squares of rate A and rate B.
		Stripes, ///< North-south stripes: the rate changes across the implement.
		Bands, ///< East-west bands: the rate changes along the driving direction.
		Gradient ///< From rate A in the west to rate B in the east.
	};

	/// @brief Adds a layer to a type 2 grid over an area, for testing without an FMIS. A grid
	/// with another area or cell size is replaced; a layer with the same variable is replaced.
	/// @param[in] cellSizeM The cell size in metres, north and east.
	/// @param[in] layer The variable the values are for (its value is ignored).
	/// @param[in] patternSizeM The size of a square, stripe or band of the pattern in metres.
	void add_test_layer(Prescription &prescription,
	                    GeoPoint southWest,
	                    GeoPoint northEast,
	                    double cellSizeM,
	                    const PrescriptionValue &layer,
	                    TestPattern pattern,
	                    std::int32_t rateA,
	                    std::int32_t rateB,
	                    double patternSizeM);

	/// @brief Adds a treatment zone bounded by one polygon, with one variable (ISO 11783-10
	/// version 4 allows one variable in a polygon zone). Returns the new zone's code.
	std::uint8_t add_polygon_zone(Prescription &prescription,
	                              const std::vector<GeoPoint> &ring,
	                              const PrescriptionValue &value,
	                              const std::string &name);

	/// @brief Metres per degree of latitude, and of longitude at a latitude.
	double metres_per_degree_latitude();
	double metres_per_degree_longitude(double latitude);
} // namespace agisotc
