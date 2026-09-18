#include "TcBridge.hpp"

#include <algorithm>
#include <cstdio>

#include <QDateTime>
#include <QFile>

#include "isobus/isobus/isobus_device_descriptor_object_pool.hpp"
#include "isobus/isobus/isobus_task_controller_client_objects.hpp"

namespace agisotc
{
	namespace
	{
		QString timestamp_now()
		{
			return QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
		}

		QString format_last_seen(std::uint64_t eventMs)
		{
			const auto now = steady_clock_ms();
			const auto ageMs = (now >= eventMs) ? (now - eventMs) : 0;
			if (ageMs < 1000)
			{
				return QString("%1 ms ago").arg(ageMs);
			}
			return QString("%1 s ago").arg(ageMs / 1000);
		}

		QString describe_object(const isobus::task_controller_object::Object &object)
		{
			using namespace isobus::task_controller_object;
			char buffer[160] = { 0 };
			if (nullptr != dynamic_cast<const DeviceObject *>(&object))
			{
				std::snprintf(buffer, sizeof(buffer), "Device %u: %s", object.get_object_id(), object.get_designator().c_str());
			}
			else if (auto *element = dynamic_cast<const DeviceElementObject *>(&object))
			{
				std::snprintf(buffer, sizeof(buffer), "Element %u (#%u, parent %u): %s",
				              object.get_object_id(), element->get_element_number(),
				              element->get_parent_object(), object.get_designator().c_str());
			}
			else if (auto *processData = dynamic_cast<const DeviceProcessDataObject *>(&object))
			{
				std::snprintf(buffer, sizeof(buffer), "Process data %u: DDI %u, triggers 0x%02X: %s",
				              object.get_object_id(), processData->get_ddi(),
				              processData->get_trigger_methods_bitfield(), object.get_designator().c_str());
			}
			else if (auto *property = dynamic_cast<const DevicePropertyObject *>(&object))
			{
				std::snprintf(buffer, sizeof(buffer), "Property %u: DDI %u: %s",
				              object.get_object_id(), property->get_ddi(), object.get_designator().c_str());
			}
			else if (nullptr != dynamic_cast<const DeviceValuePresentationObject *>(&object))
			{
				std::snprintf(buffer, sizeof(buffer), "Value presentation %u: %s",
				              object.get_object_id(), object.get_designator().c_str());
			}
			else
			{
				std::snprintf(buffer, sizeof(buffer), "Object %u: %s",
				              object.get_object_id(), object.get_designator().c_str());
			}
			return QString::fromUtf8(buffer);
		}
	} // namespace

	TcBridge::TcBridge(QObject *parent) :
	  QObject(parent)
	{
		currentSectionStates = QVariantList(currentSectionCount, QVariant(false));
		logs.addLine("AgIso Task Controller Server ready. Pick a CAN driver and press Start.");
	}

	TcBridge::~TcBridge()
	{
		stopServer();
	}

	bool TcBridge::isRunning() const
	{
		return running;
	}

	bool TcBridge::isTaskActive() const
	{
		return taskActive;
	}

	int TcBridge::selectedClient() const
	{
		return currentSelectedClient;
	}

	int TcBridge::sectionDdi() const
	{
		return currentSectionDdi;
	}

	int TcBridge::sectionCount() const
	{
		return currentSectionCount;
	}

	QVariantList TcBridge::sectionStates() const
	{
		return currentSectionStates;
	}

	QString TcBridge::statusText() const
	{
		return currentStatusText;
	}

	ClientListModel *TcBridge::clientModel()
	{
		return &clients;
	}

	DdopModel *TcBridge::ddopModel()
	{
		return &ddop;
	}

	ProcessDataModel *TcBridge::valueModel()
	{
		return &values;
	}

	LogModel *TcBridge::logModel()
	{
		return &logs;
	}

	bool TcBridge::startServer(const QString &driver, const QString &channel, int tcNumber, int booms, int sections, int channels)
	{
		if (running)
		{
			setStatus("Server is already running.");
			return false;
		}
		if ((tcNumber < 1) || (tcNumber > 32))
		{
			setStatus("TC number must be 1..32.");
			return false;
		}

		CanBusSettings settings;
		settings.driver = driver.toStdString();
		settings.channel = channel.toStdString().empty() ? "TC-Server" : channel.toStdString();
		settings.tcNumber = static_cast<std::uint8_t>(tcNumber);

		std::string error;
		if (!canBus.start(settings, error))
		{
			setStatus(QString("CAN start failed: %1").arg(QString::fromStdString(error)));
			logs.addLine(QString("[bus] %1").arg(QString::fromStdString(error)));
			return false;
		}

		auto options = isobus::TaskControllerOptions()
		                 .with_documentation()
		                 .with_implement_section_control()
		                 .with_tc_geo_with_position_based_control();

		server = std::make_shared<GuiTaskControllerServer>(
		  canBus.internal_control_function(),
		  static_cast<std::uint8_t>(booms),
		  static_cast<std::uint8_t>(sections),
		  static_cast<std::uint8_t>(channels),
		  options);
		server->get_language_command_interface().set_language_code("en");
		server->get_language_command_interface().set_country_code("US");
		server->initialize();

		pumpRunning = true;
		pumpThread = std::thread(&TcBridge::pumpLoop, this);

		running = true;
		emit runningChanged();
		setStatus(QString("Running on %1 as TC %2.").arg(QString::fromStdString(canBus.active_driver_name())).arg(tcNumber));
		logs.addLine(QString("[bus] %1").arg(currentStatusText));
		return true;
	}

	void TcBridge::stopServer()
	{
		if (!running)
		{
			return;
		}
		pumpRunning = false;
		if (pumpThread.joinable())
		{
			pumpThread.join();
		}
		if (nullptr != server)
		{
			server->terminate();
			server.reset();
		}
		canBus.stop();
		running = false;
		taskActive = false;
		emit runningChanged();
		emit taskActiveChanged();
		setStatus("Server stopped.");
		logs.addLine("[bus] Server stopped.");
	}

	void TcBridge::pumpLoop()
	{
		while (pumpRunning)
		{
			if (nullptr != server)
			{
				server->update();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}

	void TcBridge::poll()
	{
		if (!running || (nullptr == server))
		{
			return;
		}
		auto events = server->take_events();
		for (const auto &line : events.logLines)
		{
			logs.addLine(QString("[%1] %2").arg(timestamp_now(), QString::fromStdString(line)));
		}
		for (const auto &value : events.values)
		{
			values.upsertValue(value.address, value.ddi, value.element, value.value, timestamp_now());
			if ((0 != currentSectionDdi) && (value.ddi == currentSectionDdi) &&
			    (value.element >= 1) && (value.element <= currentSectionCount))
			{
				const bool on = (0 != value.value);
				if (currentSectionStates.at(value.element - 1).toBool() != on)
				{
					currentSectionStates[value.element - 1] = QVariant(on);
					emit sectionStatesChanged();
				}
			}
		}
		if (events.rosterChanged)
		{
			refreshClients();
		}
		else
		{
			// Refresh lightweight fields (last-seen, status) while keeping selection stable.
			refreshClients();
		}
		if (!events.poolsChanged.empty())
		{
			for (const auto address : events.poolsChanged)
			{
				if (static_cast<int>(address) == currentSelectedClient)
				{
					manualPool.clear();
					manualPoolClient = -1;
					refreshDdop();
				}
			}
		}
		if (events.identifyRequested)
		{
			emit identifyBanner(events.identifyNumber);
		}
	}

	void TcBridge::refreshClients()
	{
		QList<ClientRow> rows;
		for (const auto &snapshot : server->clients_snapshot())
		{
			ClientRow row;
			row.address = snapshot.address;
			row.nameHex = QString("0x%1").arg(snapshot.nameRaw, 16, 16, QChar('0')).toUpper();
			row.functionCode = snapshot.functionCode;
			row.functionInstance = snapshot.functionInstance;
			row.manufacturerCode = snapshot.manufacturerCode;
			row.identityNumber = snapshot.identityNumber;
			row.ddopSizeBytes = snapshot.ddopSizeBytes;
			row.ddopActive = snapshot.ddopActive;
			row.timedOut = snapshot.timedOut;
			row.reportedVersion = snapshot.reportedVersion;
			row.statusBits = snapshot.statusBits;
			row.lastSeen = format_last_seen(snapshot.lastSeenMs);
			rows.push_back(row);
		}
		std::sort(rows.begin(), rows.end(), [](const ClientRow &left, const ClientRow &right) { return left.address < right.address; });
		clients.setClients(rows);

		bool stillThere = false;
		for (const auto &row : rows)
		{
			if (row.address == currentSelectedClient)
			{
				stillThere = true;
			}
		}
		if (!stillThere && (-1 != currentSelectedClient))
		{
			currentSelectedClient = -1;
			emit selectedClientChanged();
			refreshDdop();
		}
	}

	void TcBridge::refreshDdop()
	{
		QList<DdopRow> rows;
		std::vector<std::uint8_t> binary;

		if ((manualPoolClient == currentSelectedClient) && !manualPool.empty())
		{
			binary = manualPool;
		}
		else if ((nullptr != server) && (-1 != currentSelectedClient))
		{
			binary = server->stored_pool(static_cast<std::uint8_t>(currentSelectedClient));
		}

		if ((-1 == currentSelectedClient))
		{
			rows.push_back({ 0, "Select a client to inspect its DDOP." });
		}
		else if (binary.empty())
		{
			rows.push_back({ 0, "No DDOP stored for this client." });
			rows.push_back({ 0, "It appears here once the client stores its pool to NVM," });
			rows.push_back({ 0, "or load a pool file manually with the button below." });
		}
		else
		{
			isobus::DeviceDescriptorObjectPool pool;
			std::vector<std::uint8_t> copy = binary;
			if (!pool.deserialize_binary_object_pool(copy))
			{
				rows.push_back({ 0, QString("Stored %1 bytes, but parsing failed.").arg(binary.size()) });
			}
			else
			{
				rows.push_back({ 0, QString("%1 objects (%2 bytes)").arg(pool.size()).arg(binary.size()) });
				for (std::uint16_t i = 0; i < pool.size(); ++i)
				{
					auto object = pool.get_object_by_index(i);
					if (nullptr == object)
					{
						continue;
					}
					int indent = 1;
					if (auto *element = dynamic_cast<isobus::task_controller_object::DeviceElementObject *>(object.get()))
					{
						indent = 1;
						auto parentId = element->get_parent_object();
						for (int depth = 0; depth < 8; ++depth)
						{
							auto parent = pool.get_object_by_id(parentId);
							if (nullptr == parent)
							{
								break;
							}
							if (nullptr != dynamic_cast<isobus::task_controller_object::DeviceElementObject *>(parent.get()))
							{
								++indent;
								parentId = dynamic_cast<isobus::task_controller_object::DeviceElementObject *>(parent.get())->get_parent_object();
							}
							else
							{
								break;
							}
						}
					}
					else if ((nullptr != dynamic_cast<isobus::task_controller_object::DeviceProcessDataObject *>(object.get())) ||
					         (nullptr != dynamic_cast<isobus::task_controller_object::DevicePropertyObject *>(object.get())) ||
					         (nullptr != dynamic_cast<isobus::task_controller_object::DeviceValuePresentationObject *>(object.get())))
					{
						indent = 2;
					}
					rows.push_back({ indent, describe_object(*object) });
				}
			}
		}
		ddop.setRows(rows);
	}

	void TcBridge::selectClient(int address)
	{
		if (currentSelectedClient != address)
		{
			currentSelectedClient = address;
			emit selectedClientChanged();
			refreshDdop();
		}
	}

	void TcBridge::setStatus(const QString &text)
	{
		if (currentStatusText != text)
		{
			currentStatusText = text;
			emit statusTextChanged();
		}
	}

	void TcBridge::setTaskActive(bool active)
	{
		if (!running || (nullptr == server))
		{
			setStatus("Start the server first.");
			return;
		}
		server->set_task_totals_active(active);
		taskActive = server->get_task_totals_active();
		emit taskActiveChanged();
		logs.addLine(QString("[task] Task totals %1.").arg(taskActive ? "ACTIVE" : "stopped"));
	}

	void TcBridge::requestValue(int ddi, int element)
	{
		if (!running || (nullptr == server))
		{
			setStatus("Start the server first.");
			return;
		}
		auto client = server->find_client(static_cast<std::uint8_t>(currentSelectedClient));
		if (nullptr == client)
		{
			setStatus("Select a client first.");
			return;
		}
		const bool sent = server->send_request_value(client, static_cast<std::uint16_t>(ddi), static_cast<std::uint16_t>(element));
		logs.addLine(QString("[cmd] Request value DDI %1 element %2 -> %3.").arg(ddi).arg(element).arg(sent ? "sent" : "FAILED"));
	}

	void TcBridge::setValue(int ddi, int element, int value, bool acknowledge)
	{
		if (!running || (nullptr == server))
		{
			setStatus("Start the server first.");
			return;
		}
		auto client = server->find_client(static_cast<std::uint8_t>(currentSelectedClient));
		if (nullptr == client)
		{
			setStatus("Select a client first.");
			return;
		}
		const bool sent = acknowledge ? server->send_set_value_and_acknowledge(client, static_cast<std::uint16_t>(ddi), static_cast<std::uint16_t>(element), static_cast<std::uint32_t>(value))
		                              : server->send_set_value(client, static_cast<std::uint16_t>(ddi), static_cast<std::uint16_t>(element), static_cast<std::uint32_t>(value));
		logs.addLine(QString("[cmd] Set value DDI %1 element %2 = %3%4 -> %5.").arg(ddi).arg(element).arg(value).arg(acknowledge ? " (ack)" : "").arg(sent ? "sent" : "FAILED"));
	}

	void TcBridge::sendMeasurement(int kind, int ddi, int element, int value)
	{
		if (!running || (nullptr == server))
		{
			setStatus("Start the server first.");
			return;
		}
		auto client = server->find_client(static_cast<std::uint8_t>(currentSelectedClient));
		if (nullptr == client)
		{
			setStatus("Select a client first.");
			return;
		}
		const auto dataDescriptionIndex = static_cast<std::uint16_t>(ddi);
		const auto elementNumber = static_cast<std::uint16_t>(element);
		const auto commandValue = static_cast<std::uint32_t>(value);
		bool sent = false;
		using ProcessDataCommand = isobus::TaskControllerServer::ProcessDataCommands;
		switch (static_cast<ProcessDataCommand>(kind))
		{
			case ProcessDataCommand::MeasurementTimeInterval:
				sent = server->send_time_interval_measurement_command(client, dataDescriptionIndex, elementNumber, commandValue);
				break;
			case ProcessDataCommand::MeasurementDistanceInterval:
				sent = server->send_distance_interval_measurement_command(client, dataDescriptionIndex, elementNumber, commandValue);
				break;
			case ProcessDataCommand::MeasurementMinimumWithinThreshold:
				sent = server->send_minimum_threshold_measurement_command(client, dataDescriptionIndex, elementNumber, commandValue);
				break;
			case ProcessDataCommand::MeasurementMaximumWithinThreshold:
				sent = server->send_maximum_threshold_measurement_command(client, dataDescriptionIndex, elementNumber, commandValue);
				break;
			case ProcessDataCommand::MeasurementChangeThreshold:
				sent = server->send_change_threshold_measurement_command(client, dataDescriptionIndex, elementNumber, commandValue);
				break;
			default:
				setStatus("Unknown measurement command.");
				return;
		}
		logs.addLine(QString("[cmd] Measurement command %1 DDI %2 element %3 = %4 -> %5.").arg(kind).arg(ddi).arg(element).arg(value).arg(sent ? "sent" : "FAILED"));
	}

	void TcBridge::setSectionDdi(int ddi)
	{
		if (currentSectionDdi != ddi)
		{
			currentSectionDdi = ddi;
			currentSectionStates = QVariantList(currentSectionCount, QVariant(false));
			emit sectionDdiChanged();
			emit sectionStatesChanged();
		}
	}

	void TcBridge::setSectionCount(int count)
	{
		count = qBound(1, count, 64);
		if (currentSectionCount != count)
		{
			currentSectionCount = count;
			currentSectionStates = QVariantList(currentSectionCount, QVariant(false));
			emit sectionCountChanged();
			emit sectionStatesChanged();
		}
	}

	void TcBridge::loadPoolFile(const QUrl &fileUrl)
	{
		if (-1 == currentSelectedClient)
		{
			setStatus("Select a client first.");
			return;
		}
		QFile file(fileUrl.toLocalFile());
		if (!file.open(QIODevice::ReadOnly))
		{
			setStatus("Could not open pool file.");
			return;
		}
		const auto data = file.readAll();
		isobus::DeviceDescriptorObjectPool pool;
		std::vector<std::uint8_t> binary(data.begin(), data.end());
		if (!pool.deserialize_binary_object_pool(binary))
		{
			setStatus("Pool file could not be parsed as a DDOP.");
			logs.addLine(QString("[ddop] Failed to parse %1.").arg(fileUrl.toLocalFile()));
			return;
		}
		manualPool = std::move(binary);
		manualPoolClient = currentSelectedClient;
		logs.addLine(QString("[ddop] Loaded %1 objects from %2 for client %3.").arg(pool.size()).arg(fileUrl.fileName()).arg(currentSelectedClient));
		refreshDdop();
	}

	void TcBridge::clearPool()
	{
		if (-1 == currentSelectedClient)
		{
			return;
		}
		manualPool.clear();
		manualPoolClient = -1;
		if (nullptr != server)
		{
			server->clear_stored_pool(static_cast<std::uint8_t>(currentSelectedClient));
		}
		refreshDdop();
	}

	void TcBridge::clearLog()
	{
		logs.clear();
	}
} // namespace agisotc
