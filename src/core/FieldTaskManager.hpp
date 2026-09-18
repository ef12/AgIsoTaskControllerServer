//================================================================================================
/// @file FieldTaskManager.hpp
///
/// @brief Field boundary, guidance line, and task management with ISOXML export.
//================================================================================================
#pragma once

#include "GpsTypes.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace agisotc
{
	/// @brief Point-in-polygon test (ray casting). Returns true if point is inside polygon.
	bool point_in_polygon(double lat, double lon, const std::vector<std::pair<double, double>> &polygon);

	/// @brief Distance from point to line segment (meters, using equirectangular approximation).
	double point_to_segment_distance(double lat, double lon,
	                                 double lat1, double lon1,
	                                 double lat2, double lon2);

	/// @brief Distance from point to polyline (meters).
	double point_to_polyline_distance(double lat, double lon,
	                                  const std::vector<std::pair<double, double>> &polyline);

	/// @brief Section control state per element
	struct SectionState
	{
		std::uint16_t element = 0;   ///< Section element number (1-based)
		bool on = false;             ///< Section on/off
		bool manualOverride = false; ///< True if manually forced
		std::uint64_t lastChangeMs = 0;
	};

	/// @brief TC-GEO section controller: evaluates position against boundaries/guidance
	/// and drives section on/off per ISO 11783-10 TC-GEO.
	class GeoSectionController
	{
	public:
		/// @brief Evaluates current position and updates section states.
		/// @param position Current GPS position (deg)
		/// @param speedMps Current speed (m/s)
		/// @param implementWidthM Total implement width (m)
		/// @param sectionWidthsM Per-section widths (m), size = sectionCount
		/// @param boundaries Active field boundaries
		/// @param guidanceLines Active guidance lines for this task
		/// @param sectionStates Output: updated section states
		/// @param sectionCount Number of sections
		static void evaluate(const GpsSolution &position,
		                     double speedMps,
		                     double implementWidthM,
		                     const std::vector<double> &sectionWidthsM,
		                     const std::vector<FieldBoundary> &boundaries,
		                     const std::vector<GuidanceLine> &guidanceLines,
		                     std::vector<SectionState> &sectionStates,
		                     std::uint8_t sectionCount);

	private:
		static bool is_inside_any_boundary(double lat, double lon,
		                                   const std::vector<FieldBoundary> &boundaries);
		static double distance_to_nearest_boundary(double lat, double lon,
		                                           const std::vector<FieldBoundary> &boundaries);
		static double distance_to_guidance_line(double lat, double lon,
		                                        const std::vector<GuidanceLine> &lines);
	};

	/// @brief Manages fields, guidance lines, tasks, and ISOXML export.
	class FieldTaskManager
	{
	public:
		using TaskDataCallback = std::function<void(const TaskDataPoint &)>;

		FieldTaskManager() = default;
		~FieldTaskManager() = default;

		// Field management
		std::string add_field(const FieldBoundary &field);
		bool remove_field(const std::string &fieldId);
		std::optional<FieldBoundary> get_field(const std::string &fieldId) const;
		std::vector<FieldBoundary> list_fields() const;

		// Guidance lines
		std::string add_guidance_line(const GuidanceLine &line);
		bool remove_guidance_line(const std::string &lineId);
		std::optional<GuidanceLine> get_guidance_line(const std::string &lineId) const;
		std::vector<GuidanceLine> list_guidance_lines(const std::string &fieldId = "") const;

		// Task management
		std::string create_task(const Task &task);
		bool start_task(const std::string &taskId);
		bool pause_task(const std::string &taskId);
		bool stop_task(const std::string &taskId);
		bool abort_task(const std::string &taskId);
		std::optional<Task> get_task(const std::string &taskId) const;
		std::vector<Task> list_tasks() const;
		std::optional<Task> get_active_task() const;

		/// @brief Called periodically with current GPS solution to log task data.
		void on_position_update(const GpsSolution &position);

		/// @brief Registers a callback for logged task data points.
		void on_task_data(TaskDataCallback cb);

		/// @brief Exports active task data to ISOXML (returns XML string).
		/// @param taskId Task to export (or active task if empty).
		/// @param[out] xmlOut Output XML string (TASKDATA.XML content).
		/// @param[out] binOut Optional binary data (TASKDATA.BIN content).
		/// @returns true on success.
		bool export_isoxml(const std::string &taskId,
		                   std::string &xmlOut,
		                   std::vector<std::uint8_t> *binOut = nullptr);

		/// @brief Imports field boundaries from ISOXML file.
		/// @returns Number of fields imported.
		std::size_t import_isoxml_fields(std::string_view xmlContent);

	private:
		mutable std::mutex mutex;
		std::map<std::string, FieldBoundary> fields;
		std::map<std::string, GuidanceLine> guidanceLines;
		std::map<std::string, Task> tasks;
		std::string activeTaskId;
		std::vector<TaskDataPoint> taskLog;
		TaskDataCallback taskDataCb;
		std::vector<SectionState> sectionStates;

		// Section control state per task
		std::map<std::string, std::vector<SectionState>> taskSectionStates;
	};
} // namespace agisotc