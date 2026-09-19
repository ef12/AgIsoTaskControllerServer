//================================================================================================
/// @file FieldTracker.hpp
///
/// @brief Tracks GPS position, records field boundaries, manages field/work state.
//================================================================================================
#pragma once

#include "GpsTypes.hpp"

#include <chrono>
#include <cmath>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace agisotc
{
	/// @brief Single recorded GPS point with timestamp
	struct TrackPoint
	{
		double latitudeDeg = 0.0;
		double longitudeDeg = 0.0;
		double altitudeM = 0.0;
		double speedMps = 0.0;
		double courseDeg = 0.0;
		std::uint64_t timestampMs = 0;
		FixQuality fixQuality = FixQuality::Invalid;
		std::uint8_t satellites = 0;
	};

	/// @brief Work tracking state
	struct WorkState
	{
		std::string fieldId;
		std::uint64_t startedMs = 0;
		std::uint64_t lastUpdateMs = 0;
		double totalWorkedHectares = 0.0;
		double currentSpeedMps = 0.0;
		bool isWorking = false;
	};

	/// @brief Section state for TC-SC
	struct SectionState
	{
		std::uint16_t elementNumber = 0; // 1-based
		bool isOn = false;
		bool isOverlapping = false;
		double workedAreaHectares = 0.0;
	};

	/// @brief Tractor state for physics simulation
	struct TractorState
	{
		double latitudeDeg = 0.0;
		double longitudeDeg = 0.0;
		double headingDeg = 0.0; // 0 = North, clockwise
		double speedMps = 0.0;
		double steeringAngleDeg = 0.0; // -max to +max
		double wheelbaseM = 3.5; // Distance between front and rear axle
	};

	/// @brief Trailed implement state
	struct ImplementState
	{
		double latitudeDeg = 0.0;
		double longitudeDeg = 0.0;
		double headingDeg = 0.0;
		double hitchOffsetM = 2.0; // Distance from tractor rear axle to hitch
		double implementLengthM = 6.0;
	};

	/// @brief Field tracker: GPS tracking, boundary recording, work tracking
	class FieldTracker
	{
	public:
		using PositionCallback = std::function<void(const TrackPoint&)>;
		using BoundaryCallback = std::function<void(const FieldBoundary&)>;
		using WorkCallback = std::function<void(const WorkState&)>;
		using SectionCallback = std::function<void(const std::vector<SectionState>&)>;

		FieldTracker();
		~FieldTracker() = default;

		FieldTracker(const FieldTracker&) = delete;
		FieldTracker& operator=(const FieldTracker&) = delete;

		// Callbacks
		void on_position_update(PositionCallback cb);
		void on_boundary_complete(BoundaryCallback cb);
		void on_work_update(WorkCallback cb);
		void on_section_update(SectionCallback cb);

		// GPS position update (called from GpsProvider)
		void update_position(const TrackPoint& point);

		// Boundary recording
		void start_boundary_recording(const std::string& fieldName);
		void stop_boundary_recording();
		bool is_recording_boundary() const;

		// Work tracking
		void start_work(const std::string& fieldId);
		void stop_work();
		void pause_work();
		bool is_working() const;

		// Section control (from TC-SC)
		void update_sections(const std::vector<SectionState>& sections);

		// Throttle/steering control (from UI)
		void set_throttle(double throttle); // 0.0 to 1.0
		void set_steering(double steering); // -1.0 to 1.0
		void emergency_stop();

		// Physics simulation step (call periodically, e.g., 50ms)
		void simulate_step(std::chrono::milliseconds dt);

		// Getters
		std::optional<TrackPoint> current_position() const;
		std::optional<FieldBoundary> current_boundary() const;
		WorkState work_state() const;
		std::vector<SectionState> section_states() const;
		TractorState tractor_state() const;
		ImplementState implement_state() const;
		std::vector<TrackPoint> track_history() const; // Last N points

		// Field management
		void add_field(const FieldBoundary& field);
		std::vector<FieldBoundary> fields() const;
		std::optional<FieldBoundary> get_field(const std::string& id) const;

	private:
		void update_tractor_physics(std::chrono::milliseconds dt);
		void update_implement_physics(std::chrono::milliseconds dt);
		void check_boundary_crossing();
		double calculate_worked_area() const;

		mutable std::mutex mutex;

		// GPS tracking
		std::optional<TrackPoint> currentPos;
		std::vector<TrackPoint> trackHistory;
		static constexpr size_t MAX_HISTORY = 10000;

		// Boundary recording
		bool recordingBoundary = false;
		std::string recordingFieldName;
		FieldBoundary currentBoundary;

		// Fields
		std::map<std::string, FieldBoundary> fieldMap;

		// Work tracking
		WorkState workState;

		// Sections
		std::vector<SectionState> sections;

		// Tractor/implement physics
		TractorState tractor;
		ImplementState implement;
		double throttle = 0.0; // 0.0 to 1.0
		double steeringInput = 0.0; // -1.0 to 1.0
		static constexpr double MAX_STEERING_DEG = 40.0;
		static constexpr double MAX_SPEED_MPS = 15.0; // ~54 km/h
		static constexpr double ACCELERATION_MPS2 = 2.0;
		static constexpr double DECELERATION_MPS2 = 3.0;

		// Callbacks
		PositionCallback positionCb;
		BoundaryCallback boundaryCb;
		WorkCallback workCb;
		SectionCallback sectionCb;
	};
} // namespace agisotc