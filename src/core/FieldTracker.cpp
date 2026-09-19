//================================================================================================
/// @file FieldTracker.cpp
///
/// @brief GPS tracking, field boundary recording, work tracking, and tractor/implement physics.
//================================================================================================
#include "FieldTracker.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace agisotc
{
	namespace
	{
		constexpr double PI = 3.14159265358979323846; // MSVC <cmath> does not provide PI

		// Haversine distance between two lat/lon points in meters
		double haversine_distance(double lat1, double lon1, double lat2, double lon2)
		{
			constexpr double R = 6371000.0; // Earth radius in meters
			double phi1 = lat1 * PI / 180.0;
			double phi2 = lat2 * PI / 180.0;
			double deltaPhi = (lat2 - lat1) * PI / 180.0;
			double deltaLambda = (lon2 - lon1) * PI / 180.0;

			double a = std::sin(deltaPhi / 2.0) * std::sin(deltaPhi / 2.0) +
			           std::cos(phi1) * std::cos(phi2) *
			           std::sin(deltaLambda / 2.0) * std::sin(deltaLambda / 2.0);
			double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
			return R * c;
		}

		// Bearing from point 1 to point 2 in degrees (0 = North)
		double bearing(double lat1, double lon1, double lat2, double lon2)
		{
			double phi1 = lat1 * PI / 180.0;
			double phi2 = lat2 * PI / 180.0;
			double deltaLambda = (lon2 - lon1) * PI / 180.0;

			double y = std::sin(deltaLambda) * std::cos(phi2);
			double x = std::cos(phi1) * std::sin(phi2) -
			           std::sin(phi1) * std::cos(phi2) * std::cos((lat2 - lat1) * PI / 180.0);
			double theta = std::atan2(y, x);
			double bearing = theta * 180.0 / PI;
			return std::fmod(bearing + 360.0, 360.0);
		}

		// Destination point given start, bearing, distance
		void destination_point(double lat1, double lon1, double bearingDeg, double distanceM,
		                       double& lat2, double& lon2)
		{
			constexpr double R = 6371000.0;
			double phi1 = lat1 * PI / 180.0;
			double lambda1 = lon1 * PI / 180.0;
			double theta = bearingDeg * PI / 180.0;
			double delta = distanceM / R;

			double phi2 = std::asin(std::sin(phi1) * std::cos(delta) +
			                        std::cos(phi1) * std::sin(delta) * std::cos(theta));
			double lambda2 = lambda1 + std::atan2(std::sin(theta) * std::sin(delta) * std::cos(phi1),
			                                      std::cos(delta) - std::sin(phi1) * std::sin(phi2));

			lat2 = phi2 * 180.0 / PI;
			lon2 = std::fmod(lambda2 * 180.0 / PI + 540.0, 360.0) - 180.0;
		}

		// Point-in-polygon (ray casting)
		bool point_in_polygon(double lat, double lon, const std::vector<std::pair<double, double>>& poly)
		{
			if (poly.size() < 3) return false;
			bool inside = false;
			for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++)
			{
				double latI = poly[i].first, lonI = poly[i].second;
				double latJ = poly[j].first, lonJ = poly[j].second;
				bool intersect = ((lonI > lon) != (lonJ > lon)) &&
				                 (lat < (latJ - latI) * (lon - lonI) / (lonJ - lonI) + latI);
				if (intersect) inside = !inside;
			}
			return inside;
		}

		// Polygon area (hectares) using shoelace formula
		double polygon_area_hectares(const std::vector<std::pair<double, double>>& poly)
		{
			if (poly.size() < 3) return 0.0;
			double area = 0.0;
			for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++)
			{
				// Convert to local metric coordinates for accurate area
				double latRef = poly[0].first * PI / 180.0;
				double x1 = poly[j].second * 111319.0 * std::cos(latRef);
				double y1 = poly[j].first * 111319.0;
				double x2 = poly[i].second * 111319.0 * std::cos(latRef);
				double y2 = poly[i].first * 111319.0;
				area += (x1 * y2 - x2 * y1);
			}
			return std::abs(area) / 2.0 / 10000.0; // m² to hectares
		}
	} // namespace

	FieldTracker::FieldTracker() :
	  tractor(),
	  implement(),
	  throttle(0.0),
	  steeringInput(0.0)
	{
		// Default tractor at origin
		tractor.latitudeDeg = 52.0;
		tractor.longitudeDeg = 5.0;
		tractor.headingDeg = 0.0;
		tractor.speedMps = 0.0;

		implement.latitudeDeg = tractor.latitudeDeg;
		implement.longitudeDeg = tractor.longitudeDeg;
		implement.headingDeg = tractor.headingDeg;
	}

	void FieldTracker::on_position_update(PositionCallback cb)
	{
		std::lock_guard<std::mutex> lock(mutex);
		positionCb = std::move(cb);
	}

	void FieldTracker::on_boundary_complete(BoundaryCallback cb)
	{
		std::lock_guard<std::mutex> lock(mutex);
		boundaryCb = std::move(cb);
	}

	void FieldTracker::on_work_update(WorkCallback cb)
	{
		std::lock_guard<std::mutex> lock(mutex);
		workCb = std::move(cb);
	}

	void FieldTracker::on_section_update(SectionCallback cb)
	{
		std::lock_guard<std::mutex> lock(mutex);
		sectionCb = std::move(cb);
	}

	void FieldTracker::update_position(const TrackPoint& point)
	{
		std::lock_guard<std::mutex> lock(mutex);

		currentPos = point;
		trackHistory.push_back(point);
		if (trackHistory.size() > MAX_HISTORY)
			trackHistory.erase(trackHistory.begin());

		// Update tractor position from GPS when not simulating
		if (throttle == 0.0 && steeringInput == 0.0)
		{
			if ((0.0 != point.latitudeDeg) || (0.0 != point.longitudeDeg))
			{
				tractor.latitudeDeg = point.latitudeDeg;
				tractor.longitudeDeg = point.longitudeDeg;
			}
			tractor.headingDeg = point.courseDeg;
			tractor.speedMps = point.speedMps;
		}

		// Boundary recording
		if (recordingBoundary && ((0.0 != point.latitudeDeg) || (0.0 != point.longitudeDeg)))
		{
			currentBoundary.exteriorRing.emplace_back(point.latitudeDeg, point.longitudeDeg);
		}

		// Work tracking
		if (workState.isWorking)
		{
			workState.lastUpdateMs = point.timestampMs;
			workState.currentSpeedMps = point.speedMps;
		}

		if (positionCb)
			positionCb(point);
	}

	void FieldTracker::start_boundary_recording(const std::string& fieldName)
	{
		std::lock_guard<std::mutex> lock(mutex);
		recordingBoundary = true;
		recordingFieldName = fieldName;
		currentBoundary = FieldBoundary();
		currentBoundary.name = fieldName;
		currentBoundary.id = "field_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
		currentBoundary.exteriorRing.clear();
		currentBoundary.createdMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	void FieldTracker::stop_boundary_recording()
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (!recordingBoundary) return;

		recordingBoundary = false;

		// Close the polygon if needed
		if (currentBoundary.exteriorRing.size() >= 3)
		{
			auto& v = currentBoundary.exteriorRing;
			if (haversine_distance(v.front().first, v.front().second, v.back().first, v.back().second) > 5.0)
			{
				v.push_back(v.front()); // Close polygon
			}
			currentBoundary.areaHectares = polygon_area_hectares(currentBoundary.exteriorRing);
			fieldMap[currentBoundary.id] = currentBoundary;

			if (boundaryCb)
				boundaryCb(currentBoundary);
		}
	}

	bool FieldTracker::is_recording_boundary() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return recordingBoundary;
	}

	void FieldTracker::start_work(const std::string& fieldId)
	{
		std::lock_guard<std::mutex> lock(mutex);
		workState.fieldId = fieldId;
		workState.startedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
		workState.lastUpdateMs = workState.startedMs;
		workState.totalWorkedHectares = 0.0;
		workState.isWorking = true;

		if (workCb)
			workCb(workState);
	}

	void FieldTracker::stop_work()
	{
		std::lock_guard<std::mutex> lock(mutex);
		workState.isWorking = false;
		if (workCb)
			workCb(workState);
	}

	void FieldTracker::pause_work()
	{
		std::lock_guard<std::mutex> lock(mutex);
		workState.isWorking = false;
		if (workCb)
			workCb(workState);
	}

	bool FieldTracker::is_working() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return workState.isWorking;
	}

	void FieldTracker::update_sections(const std::vector<SectionState>& newSections)
	{
		std::lock_guard<std::mutex> lock(mutex);
		sections = newSections;
		if (sectionCb)
			sectionCb(sections);
	}

	void FieldTracker::set_throttle(double t)
	{
		std::lock_guard<std::mutex> lock(mutex);
		throttle = std::clamp(t, 0.0, 1.0);
	}

	void FieldTracker::set_steering(double s)
	{
		std::lock_guard<std::mutex> lock(mutex);
		steeringInput = std::clamp(s, -1.0, 1.0);
	}

	void FieldTracker::emergency_stop()
	{
		std::lock_guard<std::mutex> lock(mutex);
		throttle = 0.0;
		steeringInput = 0.0;
		tractor.speedMps = 0.0;
	}

	void FieldTracker::simulate_step(std::chrono::milliseconds dt)
	{
		std::lock_guard<std::mutex> lock(mutex);
		update_tractor_physics(dt);
		update_implement_physics(dt);

		// Update current position from simulation
		if (throttle > 0.0 || steeringInput != 0.0 || tractor.speedMps > 0.1)
		{
			TrackPoint simPoint;
			simPoint.latitudeDeg = tractor.latitudeDeg;
			simPoint.longitudeDeg = tractor.longitudeDeg;
			simPoint.altitudeM = 0.0;
			simPoint.speedMps = tractor.speedMps;
			simPoint.courseDeg = tractor.headingDeg;
			simPoint.timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now().time_since_epoch()).count();
			simPoint.fixQuality = FixQuality::GpsFix;
			simPoint.satellites = 8;

			// Add to history
			trackHistory.push_back(simPoint);
			if (trackHistory.size() > MAX_HISTORY)
				trackHistory.erase(trackHistory.begin());

			currentPos = simPoint;

			// Boundary recording during simulation
			if (recordingBoundary)
			{
				currentBoundary.exteriorRing.emplace_back(tractor.latitudeDeg, tractor.longitudeDeg);
			}

			// Work tracking
			if (workState.isWorking && tractor.speedMps > 0.1)
			{
				workState.currentSpeedMps = tractor.speedMps;
				workState.lastUpdateMs = simPoint.timestampMs;
			}

			// Boundary crossing check
			check_boundary_crossing();

			if (positionCb)
				positionCb(simPoint);
		}

		if (workCb)
			workCb(workState);
		if (sectionCb)
			sectionCb(sections);
	}

	void FieldTracker::update_tractor_physics(std::chrono::milliseconds dt)
	{
		double dtSec = dt.count() / 1000.0;

		// Steering: input -1..1 maps to steering angle
		double targetSteeringDeg = steeringInput * MAX_STEERING_DEG;
		// Simple first-order steering response
		double steeringRate = 90.0; // deg/s max steering rate
		double maxSteeringChange = steeringRate * dtSec;
		double steeringDiff = targetSteeringDeg - tractor.steeringAngleDeg;
		tractor.steeringAngleDeg += std::clamp(steeringDiff, -maxSteeringChange, maxSteeringChange);

		// Speed control
		double targetSpeed = throttle * MAX_SPEED_MPS;
		double accel = (targetSpeed > tractor.speedMps) ? ACCELERATION_MPS2 : DECELERATION_MPS2;
		double speedDiff = targetSpeed - tractor.speedMps;
		double maxSpeedChange = accel * dtSec;
		tractor.speedMps += std::clamp(speedDiff, -maxSpeedChange, maxSpeedChange);
		tractor.speedMps = std::max(0.0, tractor.speedMps);

		// Heading change from steering (bicycle model)
		if (std::abs(tractor.steeringAngleDeg) > 0.1 && tractor.speedMps > 0.01)
		{
			double turnRadius = tractor.wheelbaseM / std::tan(tractor.steeringAngleDeg * PI / 180.0);
			double angularVel = tractor.speedMps / turnRadius; // rad/s
			double headingChange = angularVel * dtSec * 180.0 / PI;
			tractor.headingDeg = std::fmod(tractor.headingDeg + headingChange + 360.0, 360.0);
		}

		// Position update
		double distanceM = tractor.speedMps * dtSec;
		if (distanceM > 0.001)
		{
			destination_point(tractor.latitudeDeg, tractor.longitudeDeg,
			                  tractor.headingDeg, distanceM,
			                  tractor.latitudeDeg, tractor.longitudeDeg);
		}
	}

	void FieldTracker::update_implement_physics(std::chrono::milliseconds dt)
	{
		double dtSec = dt.count() / 1000.0;

		// Trailed implement follows tractor hitch point
		// Hitch is behind tractor rear axle
		double hitchLat, hitchLon;
		destination_point(tractor.latitudeDeg, tractor.longitudeDeg,
		                  tractor.headingDeg + 180.0, implement.hitchOffsetM,
		                  hitchLat, hitchLon);

		// Implement follows hitch with some lag (trailer physics)
		double distanceToHitch = haversine_distance(implement.latitudeDeg, implement.longitudeDeg, hitchLat, hitchLon);

		if (distanceToHitch > implement.hitchOffsetM * 1.5)
		{
			// Implement pulled toward hitch
			double bearingToHitch = bearing(implement.latitudeDeg, implement.longitudeDeg, hitchLat, hitchLon);
			implement.headingDeg = bearingToHitch;

			double moveDist = std::min(tractor.speedMps * dt.count() / 1000.0, distanceToHitch * 0.5);
			destination_point(implement.latitudeDeg, implement.longitudeDeg,
			                  implement.headingDeg, moveDist,
			                  implement.latitudeDeg, implement.longitudeDeg);
		}
		else
		{
			// Align with tractor heading when close
			implement.headingDeg = tractor.headingDeg;
		}
	}

	void FieldTracker::check_boundary_crossing()
	{
		if (!workState.isWorking || workState.fieldId.empty())
			return;

		auto it = fieldMap.find(workState.fieldId);
		if (it == fieldMap.end()) return;

		const auto& field = it->second;
		bool inside = point_in_polygon(tractor.latitudeDeg, tractor.longitudeDeg, field.exteriorRing);

		// TODO: Track entry/exit events, calculate worked area
		// For now just track if inside field
	}

	double FieldTracker::calculate_worked_area() const
	{
		// Simplified: area covered by implement width * distance traveled
		// TODO: Proper polygon union of implement swaths
		return 0.0;
	}

	std::optional<TrackPoint> FieldTracker::current_position() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return currentPos;
	}

	std::optional<FieldBoundary> FieldTracker::current_boundary() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (recordingBoundary && currentBoundary.exteriorRing.size() >= 2)
			return currentBoundary;
		return std::nullopt;
	}

	WorkState FieldTracker::work_state() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return workState;
	}

	std::vector<SectionState> FieldTracker::section_states() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return sections;
	}

	TractorState FieldTracker::tractor_state() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return tractor;
	}

	ImplementState FieldTracker::implement_state() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return implement;
	}

	std::vector<TrackPoint> FieldTracker::track_history() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return trackHistory;
	}

	void FieldTracker::add_field(const FieldBoundary& field)
	{
		std::lock_guard<std::mutex> lock(mutex);
		fieldMap[field.id] = field;
	}

	std::vector<FieldBoundary> FieldTracker::fields() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		std::vector<FieldBoundary> result;
		result.reserve(fieldMap.size());
		for (const auto& entry : fieldMap)
			result.push_back(entry.second);
		return result;
	}

	std::optional<FieldBoundary> FieldTracker::get_field(const std::string& id) const
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto it = fieldMap.find(id);
		if (it != fieldMap.end())
			return it->second;
		return std::nullopt;
	}
} // namespace agisotc