//================================================================================================
/// @file GpsTypes.hpp
///
/// @brief Shared GPS data structures for NMEA, ISO 11783, and TC-GEO.
//================================================================================================
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace agisotc
{
	/// @brief GPS fix quality (NMEA GGA)
	enum class FixQuality : std::uint8_t
	{
		Invalid = 0,
		GpsFix = 1,
		DgpsFix = 2,
		PpsFix = 3,
		RtkFixed = 4,
		RtkFloat = 5,
		Estimated = 6,
		Manual = 7,
		Simulation = 8
	};

	/// @brief GNSS system identifier
	enum class GnssSystem : std::uint8_t
	{
		Gps = 1,
		Glonass = 2,
		Galileo = 3,
		BeiDou = 4,
		Qzss = 5,
		NavIC = 6
	};

	/// @brief Unified position/velocity/time solution.
	struct GpsSolution
	{
		std::uint64_t timestampMs = 0;          ///< Monotonic timestamp (steady_clock)
		std::optional<double> latitudeDeg;      ///< WGS84 latitude, degrees
		std::optional<double> longitudeDeg;     ///< WGS84 longitude, degrees
		std::optional<double> altitudeM;        ///< Ellipsoidal altitude, meters
		std::optional<double> heightMslM;       ///< Mean sea level height, meters
		std::optional<double> speedMps;         ///< Speed over ground, m/s
		std::optional<double> courseDeg;        ///< Course over ground, degrees true
		std::optional<double> hdop;             ///< Horizontal dilution of precision
		std::optional<double> vdop;             ///< Vertical dilution of precision
		std::optional<double> pdop;             ///< Position dilution of precision
		std::optional<std::uint8_t> satellites; ///< Number of satellites used
		std::optional<FixQuality> fixQuality;   ///< Fix quality indicator
		std::optional<double> ageOfDgpsS;       ///< Age of differential corrections, seconds
		std::optional<std::uint16_t> dgpsStationId; ///< DGPS station ID
		bool valid = false;                     ///< True if position is valid

		void clear()
		{
			latitudeDeg.reset();
			longitudeDeg.reset();
			altitudeM.reset();
			heightMslM.reset();
			speedMps.reset();
			courseDeg.reset();
			hdop.reset();
			vdop.reset();
			pdop.reset();
			satellites.reset();
			fixQuality.reset();
			ageOfDgpsS.reset();
			dgpsStationId.reset();
			valid = false;
		}
	};

	/// @brief Satellite info (from GSA/GSV)
	struct SatelliteInfo
	{
		std::uint8_t prn = 0;
		GnssSystem system = GnssSystem::Gps;
		std::uint8_t elevationDeg = 0;
		std::uint16_t azimuthDeg = 0;
		std::uint8_t snrDbHz = 0;
		bool usedInFix = false;
	};

	/// @brief GNSS status snapshot
	struct GnssStatus
	{
		std::vector<SatelliteInfo> satellites;
		std::uint8_t satellitesInView = 0;
		std::uint8_t satellitesUsed = 0;
	};

	/// @brief Field boundary polygon (ISOXML Part 4 compatible)
	struct FieldBoundary
	{
		std::string id;
		std::string name;
		std::vector<std::pair<double, double>> exteriorRing; ///< (lat, lon) in degrees, CCW
		std::vector<std::vector<std::pair<double, double>>> interiorRings; ///< Holes, CW
		std::uint64_t createdMs = 0;
		double areaHectares = 0.0; ///< Cached polygon area, filled by FieldTracker on record
	};

	/// @brief Guidance line (AB line or curve)
	struct GuidanceLine
	{
		std::string id;
		std::string name;
		enum class Type { ABLine, ACurve, Boundary } type = Type::ABLine;
		std::vector<std::pair<double, double>> points; ///< (lat, lon) in degrees
		double swathWidthM = 0.0; ///< Implement width for this line
		std::string fieldId;      ///< Associated field
	};

	/// @brief Task definition
	struct Task
	{
		std::string id;
		std::string name;
		std::string fieldId;
		std::string clientId;               ///< Associated TC client address
		std::uint64_t createdMs = 0;
		std::uint64_t startedMs = 0;
		std::uint64_t stoppedMs = 0;
		enum class State { Created, Active, Paused, Completed, Aborted } state = State::Created;
		std::vector<std::uint16_t> ddIsToLog; ///< DDI list to record
		std::uint32_t logIntervalMs = 1000;   ///< Logging interval
	};

	/// @brief Single logged task data point
	struct TaskDataPoint
	{
		std::uint64_t timestampMs = 0;
		double latitudeDeg = 0.0;
		double longitudeDeg = 0.0;
		double altitudeM = 0.0;
		double speedMps = 0.0;
		double courseDeg = 0.0;
		std::vector<std::pair<std::uint16_t, std::int32_t>> ddiValues; ///< (DDI, value)
	};
} // namespace agisotc