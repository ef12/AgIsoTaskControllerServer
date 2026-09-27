#include "CanBusManager.hpp"
#include "FieldTaskManager.hpp"
#include "GpsProvider.hpp"

#include <cassert>
#include <chrono>
#include <string>
#include <thread>

int main()
{
	agisotc::GpsProvider gps;
	gps.configure_simulation(52.0, 5.0, 10.0, 90.0);
	gps.update();
	const auto initial = gps.current_solution();
	std::this_thread::sleep_for(std::chrono::milliseconds(120));
	gps.update();
	const auto moved = gps.current_solution();
	assert(initial.valid && moved.valid);
	assert(initial.longitudeDeg && moved.longitudeDeg);
	assert(*moved.longitudeDeg > *initial.longitudeDeg);

	agisotc::FieldTaskManager manager;
	agisotc::FieldBoundary field;
	field.name = "Test Field";
	field.exteriorRing = {
		{ 51.999, 4.999 },
		{ 51.999, 5.001 },
		{ 52.001, 5.001 },
		{ 52.001, 4.999 },
		{ 51.999, 4.999 }
	};
	const std::string fieldId = manager.add_field(field);
	assert(!fieldId.empty());
	assert(agisotc::point_in_polygon(52.0, 5.0, field.exteriorRing));

	agisotc::Task task;
	task.name = "Test Task";
	task.fieldId = fieldId;
	const std::string taskId = manager.create_task(task);
	assert(!taskId.empty());
	assert(manager.start_task(taskId));
	assert(manager.get_active_task().has_value());

	int loggedPoints = 0;
	manager.on_task_data([&loggedPoints](const agisotc::TaskDataPoint &) { ++loggedPoints; });
	manager.on_position_update(moved);
	assert(1 == loggedPoints);
	assert(manager.stop_task(taskId));
	assert(!manager.get_active_task().has_value());

	// A CAN-API 2 network name is limited to 20 bytes; this must fail before any driver is opened.
	agisotc::CanBusManager canBus;
	agisotc::CanBusSettings pcanVirtual;
	pcanVirtual.driver = "pcan_virtual";
	pcanVirtual.channel = "network_name_over_20_bytes";
	std::string error;
	assert(!canBus.start(pcanVirtual, error));
	assert(!canBus.is_running());
#if defined(ISOBUS_WINDOWSCANAPI2_AVAILABLE)
	assert(std::string::npos != error.find("between 1 and 20 bytes"));
#else
	assert(std::string::npos != error.find("not available"));
#endif
	return 0;
}
