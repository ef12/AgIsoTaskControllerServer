#include "FieldTaskManager.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace agisotc
{
	namespace
	{
		constexpr double EarthRadiusM = 6371000.0;
		constexpr double DegreesToRadians = 3.14159265358979323846 / 180.0;
		std::atomic_uint64_t nextId = { 1 };

		std::uint64_t now_ms()
		{
			return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			  std::chrono::system_clock::now().time_since_epoch()).count());
		}

		std::string make_id(const char *prefix)
		{
			return std::string(prefix) + std::to_string(nextId.fetch_add(1));
		}

		double local_x(double latitude, double longitude, double referenceLatitude, double referenceLongitude)
		{
			return (longitude - referenceLongitude) * DegreesToRadians * EarthRadiusM *
			       std::cos(referenceLatitude * DegreesToRadians);
		}

		double local_y(double latitude, double referenceLatitude)
		{
			return (latitude - referenceLatitude) * DegreesToRadians * EarthRadiusM;
		}

		std::string xml_escape(const std::string &value)
		{
			std::string escaped;
			for (const char character : value)
			{
				switch (character)
				{
					case '&': escaped += "&amp;"; break;
					case '<': escaped += "&lt;"; break;
					case '>': escaped += "&gt;"; break;
					case '"': escaped += "&quot;"; break;
					default: escaped += character; break;
				}
			}
			return escaped;
		}
	} // namespace

	bool point_in_polygon(double latitude, double longitude, const std::vector<std::pair<double, double>> &polygon)
	{
		if (polygon.size() < 3)
		{
			return false;
		}
		bool inside = false;
		for (std::size_t current = 0, previous = polygon.size() - 1; current < polygon.size(); previous = current++)
		{
			const auto &[currentLat, currentLon] = polygon[current];
			const auto &[previousLat, previousLon] = polygon[previous];
			const bool crosses = ((currentLat > latitude) != (previousLat > latitude)) &&
			                     (longitude < ((previousLon - currentLon) * (latitude - currentLat) /
			                                     (previousLat - currentLat) + currentLon));
			if (crosses)
			{
				inside = !inside;
			}
		}
		return inside;
	}

	double point_to_segment_distance(double latitude, double longitude,
	                                 double latitude1, double longitude1,
	                                 double latitude2, double longitude2)
	{
		const double x = local_x(latitude, longitude, latitude1, longitude1);
		const double y = local_y(latitude, latitude1);
		const double segmentX = local_x(latitude2, longitude2, latitude1, longitude1);
		const double segmentY = local_y(latitude2, latitude1);
		const double lengthSquared = (segmentX * segmentX) + (segmentY * segmentY);
		const double ratio = (lengthSquared > 0.0) ? std::clamp(((x * segmentX) + (y * segmentY)) / lengthSquared, 0.0, 1.0) : 0.0;
		return std::hypot(x - (ratio * segmentX), y - (ratio * segmentY));
	}

	double point_to_polyline_distance(double latitude, double longitude,
	                                  const std::vector<std::pair<double, double>> &polyline)
	{
		if (polyline.size() < 2)
		{
			return 0.0;
		}
		double nearest = std::numeric_limits<double>::max();
		for (std::size_t index = 1; index < polyline.size(); ++index)
		{
			nearest = std::min(nearest, point_to_segment_distance(latitude, longitude,
			                                                       polyline[index - 1].first, polyline[index - 1].second,
			                                                       polyline[index].first, polyline[index].second));
		}
		return nearest;
	}

	bool GeoSectionController::is_inside_any_boundary(double latitude, double longitude,
	                                                  const std::vector<FieldBoundary> &boundaries)
	{
		return std::any_of(boundaries.begin(), boundaries.end(), [latitude, longitude](const FieldBoundary &boundary) {
			return point_in_polygon(latitude, longitude, boundary.exteriorRing);
		});
	}

	double GeoSectionController::distance_to_nearest_boundary(double latitude, double longitude,
	                                                          const std::vector<FieldBoundary> &boundaries)
	{
		double nearest = std::numeric_limits<double>::max();
		for (const auto &boundary : boundaries)
		{
			nearest = std::min(nearest, point_to_polyline_distance(latitude, longitude, boundary.exteriorRing));
		}
		return nearest;
	}

	double GeoSectionController::distance_to_guidance_line(double latitude, double longitude,
	                                                       const std::vector<GuidanceLine> &lines)
	{
		double nearest = std::numeric_limits<double>::max();
		for (const auto &line : lines)
		{
			nearest = std::min(nearest, point_to_polyline_distance(latitude, longitude, line.points));
		}
		return nearest;
	}

	void GeoSectionController::evaluate(const GpsSolution &position,
	                                    double,
	                                    double,
	                                    const std::vector<double> &,
	                                    const std::vector<FieldBoundary> &boundaries,
	                                    const std::vector<GuidanceLine> &,
	                                    std::vector<SectionState> &states,
	                                    std::uint8_t sectionCount)
	{
		states.resize(sectionCount);
		const bool inside = position.valid && position.latitudeDeg && position.longitudeDeg &&
		                    is_inside_any_boundary(*position.latitudeDeg, *position.longitudeDeg, boundaries);
		for (std::uint16_t index = 0; index < sectionCount; ++index)
		{
			states[index].element = static_cast<std::uint16_t>(index + 1);
			if (!states[index].manualOverride)
			{
				states[index].on = inside;
			}
		}
	}

	std::string FieldTaskManager::add_field(const FieldBoundary &field)
	{
		if (field.name.empty() || (field.exteriorRing.size() < 3))
		{
			return {};
		}
		FieldBoundary stored = field;
		if (stored.id.empty())
		{
			stored.id = make_id("FLD-");
		}
		if (0 == stored.createdMs)
		{
			stored.createdMs = now_ms();
		}
		std::lock_guard<std::mutex> lock(mutex);
		fields[stored.id] = stored;
		return stored.id;
	}

	bool FieldTaskManager::remove_field(const std::string &fieldId)
	{
		std::lock_guard<std::mutex> lock(mutex);
		return 0 != fields.erase(fieldId);
	}

	std::optional<FieldBoundary> FieldTaskManager::get_field(const std::string &fieldId) const
	{
		std::lock_guard<std::mutex> lock(mutex);
		const auto result = fields.find(fieldId);
		return (fields.end() == result) ? std::optional<FieldBoundary>() : std::optional<FieldBoundary>(result->second);
	}

	std::vector<FieldBoundary> FieldTaskManager::list_fields() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		std::vector<FieldBoundary> result;
		for (const auto &[id, field] : fields)
		{
			result.push_back(field);
		}
		return result;
	}

	std::string FieldTaskManager::add_guidance_line(const GuidanceLine &line)
	{
		if (line.name.empty() || (line.points.size() < 2))
		{
			return {};
		}
		GuidanceLine stored = line;
		if (stored.id.empty())
		{
			stored.id = make_id("GPN-");
		}
		std::lock_guard<std::mutex> lock(mutex);
		guidanceLines[stored.id] = stored;
		return stored.id;
	}

	bool FieldTaskManager::remove_guidance_line(const std::string &lineId)
	{
		std::lock_guard<std::mutex> lock(mutex);
		return 0 != guidanceLines.erase(lineId);
	}

	std::optional<GuidanceLine> FieldTaskManager::get_guidance_line(const std::string &lineId) const
	{
		std::lock_guard<std::mutex> lock(mutex);
		const auto result = guidanceLines.find(lineId);
		return (guidanceLines.end() == result) ? std::optional<GuidanceLine>() : std::optional<GuidanceLine>(result->second);
	}

	std::vector<GuidanceLine> FieldTaskManager::list_guidance_lines(const std::string &fieldId) const
	{
		std::lock_guard<std::mutex> lock(mutex);
		std::vector<GuidanceLine> result;
		for (const auto &[id, line] : guidanceLines)
		{
			if (fieldId.empty() || (line.fieldId == fieldId))
			{
				result.push_back(line);
			}
		}
		return result;
	}

	std::string FieldTaskManager::create_task(const Task &task)
	{
		if (task.name.empty())
		{
			return {};
		}
		Task stored = task;
		if (stored.id.empty())
		{
			stored.id = make_id("TSK-");
		}
		if (0 == stored.createdMs)
		{
			stored.createdMs = now_ms();
		}
		std::lock_guard<std::mutex> lock(mutex);
		if (!stored.fieldId.empty() && (fields.end() == fields.find(stored.fieldId)))
		{
			return {};
		}
		tasks[stored.id] = stored;
		return stored.id;
	}

	bool FieldTaskManager::start_task(const std::string &taskId)
	{
		std::lock_guard<std::mutex> lock(mutex);
		const auto result = tasks.find(taskId);
		if (tasks.end() == result)
		{
			return false;
		}
		if (!activeTaskId.empty() && (activeTaskId != taskId))
		{
			tasks[activeTaskId].state = Task::State::Paused;
		}
		activeTaskId = taskId;
		result->second.state = Task::State::Active;
		result->second.startedMs = now_ms();
		return true;
	}

	bool FieldTaskManager::pause_task(const std::string &taskId)
	{
		std::lock_guard<std::mutex> lock(mutex);
		const auto result = tasks.find(taskId);
		if ((tasks.end() == result) || (result->second.state != Task::State::Active))
		{
			return false;
		}
		result->second.state = Task::State::Paused;
		activeTaskId.clear();
		return true;
	}

	bool FieldTaskManager::stop_task(const std::string &taskId)
	{
		std::lock_guard<std::mutex> lock(mutex);
		const auto result = tasks.find(taskId);
		if (tasks.end() == result)
		{
			return false;
		}
		result->second.state = Task::State::Completed;
		result->second.stoppedMs = now_ms();
		if (activeTaskId == taskId)
		{
			activeTaskId.clear();
		}
		return true;
	}

	bool FieldTaskManager::abort_task(const std::string &taskId)
	{
		std::lock_guard<std::mutex> lock(mutex);
		const auto result = tasks.find(taskId);
		if (tasks.end() == result)
		{
			return false;
		}
		result->second.state = Task::State::Aborted;
		result->second.stoppedMs = now_ms();
		if (activeTaskId == taskId)
		{
			activeTaskId.clear();
		}
		return true;
	}

	std::optional<Task> FieldTaskManager::get_task(const std::string &taskId) const
	{
		std::lock_guard<std::mutex> lock(mutex);
		const auto result = tasks.find(taskId);
		return (tasks.end() == result) ? std::optional<Task>() : std::optional<Task>(result->second);
	}

	std::vector<Task> FieldTaskManager::list_tasks() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		std::vector<Task> result;
		for (const auto &[id, task] : tasks)
		{
			result.push_back(task);
		}
		return result;
	}

	std::optional<Task> FieldTaskManager::get_active_task() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		const auto result = tasks.find(activeTaskId);
		return (tasks.end() == result) ? std::optional<Task>() : std::optional<Task>(result->second);
	}

	void FieldTaskManager::on_position_update(const GpsSolution &position)
	{
		if (!position.valid || !position.latitudeDeg || !position.longitudeDeg)
		{
			return;
		}
		TaskDataPoint point;
		TaskDataCallback callback;
		{
			std::lock_guard<std::mutex> lock(mutex);
			const auto active = tasks.find(activeTaskId);
			if ((tasks.end() == active) || (active->second.state != Task::State::Active))
			{
				return;
			}
			if (!taskLog.empty() && ((position.timestampMs - taskLog.back().timestampMs) < active->second.logIntervalMs))
			{
				return;
			}
			point.timestampMs = position.timestampMs;
			point.latitudeDeg = *position.latitudeDeg;
			point.longitudeDeg = *position.longitudeDeg;
			point.altitudeM = position.altitudeM.value_or(0.0);
			point.speedMps = position.speedMps.value_or(0.0);
			point.courseDeg = position.courseDeg.value_or(0.0);
			taskLog.push_back(point);
			callback = taskDataCb;
		}
		if (callback)
		{
			callback(point);
		}
	}

	void FieldTaskManager::on_task_data(TaskDataCallback callback)
	{
		std::lock_guard<std::mutex> lock(mutex);
		taskDataCb = std::move(callback);
	}

	bool FieldTaskManager::export_isoxml(const std::string &taskId, std::string &xmlOut,
	                                    std::vector<std::uint8_t> *binOut)
	{
		std::lock_guard<std::mutex> lock(mutex);
		const std::string selectedId = taskId.empty() ? activeTaskId : taskId;
		const auto selected = tasks.find(selectedId);
		if (tasks.end() == selected)
		{
			return false;
		}
		std::ostringstream xml;
		xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		    << "<ISO11783_TaskData VersionMajor=\"4\" VersionMinor=\"3\">\n"
		    << "  <TSK A=\"" << xml_escape(selected->second.id) << "\" B=\""
		    << xml_escape(selected->second.name) << "\" E=\"" << xml_escape(selected->second.fieldId) << "\"/>\n"
		    << "</ISO11783_TaskData>\n";
		xmlOut = xml.str();
		if (nullptr != binOut)
		{
			std::ostringstream data;
			data << std::setprecision(12);
			for (const auto &point : taskLog)
			{
				data << point.timestampMs << ',' << point.latitudeDeg << ',' << point.longitudeDeg << ','
				     << point.altitudeM << ',' << point.speedMps << ',' << point.courseDeg << '\n';
			}
			const std::string bytes = data.str();
			binOut->assign(bytes.begin(), bytes.end());
		}
		return true;
	}

	std::size_t FieldTaskManager::import_isoxml_fields(std::string_view)
	{
		return 0;
	}
} // namespace agisotc
