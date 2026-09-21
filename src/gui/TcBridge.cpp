#include "TcBridge.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <map>

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>

#include "isobus/isobus/can_message.hpp"
#include "isobus/isobus/can_NAME.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_device_descriptor_object_pool.hpp"
#include "isobus/isobus/nmea2000_message_interface.hpp"
#include "isobus/isobus/isobus_speed_distance_messages.hpp"
#include "isobus/isobus/isobus_task_controller_client_objects.hpp"
#include "isobus/isobus/isobus_standard_data_description_indices.hpp"

namespace agisotc
{
	namespace
	{
		constexpr double EarthRadiusM = 6371000.0;
		constexpr double DegreesToRadians = 3.14159265358979323846 / 180.0;

		QString pgnName(std::uint32_t pgn)
		{
			switch (pgn)
			{
				case 0xEE00: return "AddrClaim";
				case 0xEA00: return "Request";
				case 0xEC00: return "TP.CM";
				case 0xEB00: return "TP.DT";
				case 0xC800: return "ETP.CM";
				case 0xC700: return "ETP.DT";
				case 0xCB00: return "ProcData";
				case 0xF022: return "MSSpeed";
				case 0xFD43: return "MSSpdCmd";
				case 0xFE48: return "WhlSpd";
				case 0xFE49: return "GndSpd";
				case 129025: return "GPS.Pos";
				case 129026: return "GPS.COG/SOG";
				case 129027: return "GPS.dPos";
				case 129029: return "GPS.GNSS";
				case 127250: return "VesselHdg";
				case 127251: return "ROT";
				default: return {};
			}
		}

		isobus::NMEA2000Messages::GNSSPositionData::GNSSMethod mapFixQualityToGnssMethod(const std::optional<FixQuality> &quality)
		{
			using GNSSMethod = isobus::NMEA2000Messages::GNSSPositionData::GNSSMethod;
			if (!quality.has_value())
			{
				return GNSSMethod::NoGNSS;
			}
			switch (*quality)
			{
				case FixQuality::GpsFix:
					return GNSSMethod::GNSSFix;
				case FixQuality::DgpsFix:
					return GNSSMethod::DGNSSFix;
				case FixQuality::RtkFixed:
					return GNSSMethod::RTKFixedInteger;
				case FixQuality::RtkFloat:
					return GNSSMethod::RTKFloat;
				default:
					return GNSSMethod::GNSSFix;
			}
		}
		constexpr std::uint32_t GpsPositionPgn = 65267;
		constexpr std::uint32_t GpsPositionDeltaPgn = 65268;
		constexpr std::uint32_t GpsPositionDeltaHighPrecisionPgn = 65269;
		constexpr std::uint32_t GpsPositionCovariancePgn = 65270;
		constexpr std::uint32_t GpsPositionDeltaCovariancePgn = 65271;

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

		double geometry_value_metres(isobus::DeviceDescriptorObjectPool &pool,
		                             std::int32_t rawValue,
		                             std::uint16_t presentationId)
		{
			double value = static_cast<double>(rawValue);
			if (isobus::NULL_OBJECT_ID != presentationId)
			{
				auto presentationObject = pool.get_object_by_id(presentationId);
				if (auto *presentation = dynamic_cast<isobus::task_controller_object::DeviceValuePresentationObject *>(presentationObject.get()))
				{
					value = (value + presentation->get_offset()) * presentation->get_scale();
					const QString unit = QString::fromStdString(presentation->get_designator()).trimmed().toLower();
					if ((unit == "mm") || unit.contains("millimet"))
					{
						value /= 1000.0;
					}
					else if ((unit == "cm") || unit.contains("centimet"))
					{
						value /= 100.0;
					}
					return value;
				}
			}
			// ISO 11783 geometry values without a presentation are expressed in millimetres.
			return value / 1000.0;
		}

		QString trigger_text(std::uint8_t triggers)
		{
			QStringList result;
			using Trigger = isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods;
			if (triggers & static_cast<std::uint8_t>(Trigger::TimeInterval)) result << "time";
			if (triggers & static_cast<std::uint8_t>(Trigger::DistanceInterval)) result << "distance";
			if (triggers & static_cast<std::uint8_t>(Trigger::ThresholdLimits)) result << "threshold";
			if (triggers & static_cast<std::uint8_t>(Trigger::OnChange)) result << "change";
			if (triggers & static_cast<std::uint8_t>(Trigger::Total)) result << "total";
			return result.isEmpty() ? "request" : result.join(", ");
		}

		QString tc_basic_label(std::uint16_t ddi)
		{
			using DDI = isobus::DataDescriptionIndex;
			switch (static_cast<DDI>(ddi))
			{
				case DDI::ActualWorkState: return "Work state";
				case DDI::ActualVolumePerAreaApplicationRate: return "Actual volume rate";
				case DDI::ActualMassPerAreaApplicationRate: return "Actual mass rate";
				case DDI::ActualCountPerAreaApplicationRate: return "Actual count rate";
				case DDI::ApplicationTotalVolume_L: return "Applied volume";
				case DDI::ApplicationTotalMass_kg: return "Applied mass";
				case DDI::ApplicationTotalCount: return "Applied count";
				case DDI::TotalArea: return "Worked area";
				case DDI::EffectiveTotalDistance: return "Working distance";
				case DDI::EffectiveTotalTime: return "Working time";
				case DDI::ActualPercentageApplicationRate: return "Actual application rate";
				case DDI::LoadedTotalMass: return "Loaded mass";
				case DDI::UnloadedTotalMass: return "Unloaded mass";
				default: return {};
			}
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
		stopGps();
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

	bool TcBridge::isGpsRunning() const
	{
		return gpsRunningFlag;
	}

	bool TcBridge::isGpsValid() const
	{
		return gpsRunningFlag && currentGps.valid;
	}

	QString TcBridge::gpsSourceText() const
	{
		return currentGpsSourceText;
	}

	double TcBridge::gpsLatitude() const
	{
		return currentGps.latitudeDeg.value_or(0.0);
	}

	double TcBridge::gpsLongitude() const
	{
		return currentGps.longitudeDeg.value_or(0.0);
	}

	double TcBridge::gpsSpeedKph() const
	{
		return currentGps.speedMps.value_or(0.0) * 3.6;
	}

	double TcBridge::gpsCourse() const
	{
		return currentGps.courseDeg.value_or(0.0);
	}

	double TcBridge::tractorX() const
	{
		return currentTractorX;
	}

	double TcBridge::tractorZ() const
	{
		return currentTractorZ;
	}

	QVariantList TcBridge::trackPoints() const
	{
		return currentTrackPoints;
	}
	QVariantList TcBridge::workedPoints() const { return currentWorkedPoints; }
	QVariantList TcBridge::fieldBoundaryPoints() const { return currentFieldBoundaryPoints; }
	bool TcBridge::boundaryRecording() const { return boundaryRecordingFlag; }
	int TcBridge::boundaryPointCount() const { return static_cast<int>(recordedBoundary.size()); }

	QStringList TcBridge::fieldNames() const
	{
		return currentFieldNames;
	}

	QStringList TcBridge::taskNames() const
	{
		return currentTaskNames;
	}

	int TcBridge::selectedFieldIndex() const
	{
		return currentSelectedField;
	}

	int TcBridge::selectedTaskIndex() const
	{
		return currentSelectedTask;
	}

	QString TcBridge::activeFieldName() const
	{
		return currentActiveFieldName;
	}

	QString TcBridge::activeTaskName() const
	{
		return currentActiveTaskName;
	}

	double TcBridge::fieldWidthM() const
	{
		return currentFieldWidthM;
	}

	double TcBridge::fieldLengthM() const
	{
		return currentFieldLengthM;
	}

	QString TcBridge::implementName() const { return currentImplementName; }
	QString TcBridge::implementGeometryStatus() const { return currentImplementGeometryStatus; }
	QVariantList TcBridge::implementElements() const { return currentImplementElements; }
	QVariantList TcBridge::implementDdis() const { return currentImplementDdis; }
	bool TcBridge::autoDdiSync() const { return autoDdiSyncEnabled; }
	int TcBridge::ddiSyncIntervalMs() const { return currentDdiSyncIntervalMs; }
	bool TcBridge::liveDdiTrafficWatch() const { return liveDdiTrafficWatchEnabled; }
	QVariantList TcBridge::tcBasicData() const { return currentTcBasicData; }
	int TcBridge::activeSectionCount() const
	{
		return static_cast<int>(std::count_if(currentSectionStates.cbegin(), currentSectionStates.cend(),
		                                      [](const QVariant &value) { return value.toBool(); }));
	}
	double TcBridge::workedAreaHa() const { return currentWorkedAreaHa; }
	double TcBridge::workedDistanceM() const { return currentWorkedDistanceM; }
	double TcBridge::workedTimeSeconds() const { return currentWorkedTimeSeconds; }
	double TcBridge::steeringAngle() const { return currentSteeringAngle; }
	double TcBridge::throttleKph() const { return currentThrottleKph; }
	double TcBridge::implementX() const { return currentImplementX; }
	double TcBridge::implementZ() const { return currentImplementZ; }
	double TcBridge::implementCourse() const { return currentImplementCourse; }

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

	DdiTrafficModel *TcBridge::ddiTrafficModel()
	{
		return &ddiTraffic;
	}

	BusMonitorModel *TcBridge::busMonitorModel()
	{
		return &busMonitor;
	}

	void TcBridge::clearBusMonitor()
	{
		busMonitor.clear();
	}

	void TcBridge::drainBusFrames()
	{
		if (!canBus.is_running())
		{
			return;
		}
		for (const auto &frame : canBus.take_sniffed_frames())
		{
			BusFrameRow row;
			row.timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
			row.direction = frame.outgoing ? "TX" : "RX";
			std::uint32_t pgn = frame.identifier;
			int source = -1;
			int destination = 255;
			if (frame.identifier > 0x7FF)
			{
				pgn = (frame.identifier >> 8) & 0x3FFFF;
				source = static_cast<int>(frame.identifier & 0xFF);
				const std::uint32_t format = (frame.identifier >> 16) & 0xFF;
				if (format < 0xF0)
				{
					pgn = (frame.identifier >> 8) & 0x3FF00;
					destination = static_cast<int>((frame.identifier >> 8) & 0xFF);
				}
			}
			const QString name = pgnName(pgn);
			row.pgn = name.isEmpty() ? QString("0x%1 (%2)").arg(pgn, 5, 16, QChar('0')).arg(pgn).toUpper()
			                         : QString("0x%1 (%2) %3").arg(pgn, 5, 16, QChar('0')).arg(pgn).arg(name).toUpper();
			row.source = source;
			row.destination = destination;
			row.length = frame.length;
			QString hex;
			hex.reserve(24);
			for (std::uint8_t i = 0; (i < frame.length) && (i < 8); ++i)
			{
				if (0 != i)
				{
					hex += ' ';
				}
				hex += QString("%1").arg(frame.data[i], 2, 16, QChar('0')).toUpper();
			}
			if ((0xEE00 == pgn) && (frame.length >= 8))
			{
				// Decode the claimed NAME so the bus tab shows WHO claimed.
				std::uint64_t rawName = 0;
				for (std::uint8_t i = 0; i < 8; ++i)
				{
					rawName |= static_cast<std::uint64_t>(frame.data[i]) << (8 * i);
				}
				const isobus::NAME claimed(rawName);
				hex += QString(" [func=%1 inst=%2 mfr=%3]")
				         .arg(claimed.get_function_code())
				         .arg(claimed.get_function_instance())
				         .arg(claimed.get_manufacturer_code());
			}
			row.data = hex;
			busMonitor.addRow(row);
		}
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
		registerGpsCanCallbacks();
		if ("virtual" == settings.driver)
		{
			logs.addLine("[bus] WARNING: the 'virtual' driver is process-local. External simulators "
			             "on this machine cannot join it - use 'wcan' with the same bus name instead.");
		}

		// Broadcast our GPS/simulated motion as machine speed so implements
		// and terminals on the bus pick up speed and distance. We sense the
		// speed ourselves (GPS/simulator), hence periodic transmission of the
		// ground-based, wheel-based, and machine-selected speed messages.
		speedMessages = std::make_unique<isobus::SpeedMessagesInterface>(
		  canBus.internal_control_function(),
		  true,
		  true,
		  true,
		  false);
		currentMachineDistanceMm = 0;
		speedMessages->initialize();

		// Bridge our GPS (receiver or simulator) onto the bus as NMEA 2000
		// so implements that listen for NMEA 2000 GPS get position, course,
		// and speed: 129025 position rapid, 129026 COG/SOG, 129029 GNSS fix.
		nmea2000 = std::make_unique<isobus::NMEA2000MessageInterface>(
		  canBus.internal_control_function(),
		  true,
		  false,
		  true,
		  false,
		  true,
		  false,
		  false);
		nmea2000->initialize();

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
		server->set_task_totals_active(taskActive);

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
		speedMessages.reset();
		nmea2000.reset();
		unregisterGpsCanCallbacks();
		canBus.stop();
		for (auto &state : implementDdiStates) state.reportingConfigured = false;
		lastDdiSyncMs = 0;
		running = false;
		emit runningChanged();
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
		drainBusFrames();
		updateGps();
		if (!gpsRunningFlag)
		{
			updateSpeedMessages(0.0);
		}
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
			if (static_cast<int>(value.address) == currentSelectedClient)
			{
				updateImplementValue(value.ddi, value.element, value.value);
			}
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
		if (liveDdiTrafficWatchEnabled)
		{
			for (const auto &event : events.ddiTraffic)
			{
				if (event.acknowledge)
				{
					appendDdiTraffic("client -> TC", "PDACK", event.address, event.ddi, event.element, event.errorCode,
					                 QString("command %1").arg(event.command));
				}
				else if (event.hasValue)
				{
					appendDdiTraffic("client -> TC", "Value", event.address, event.ddi, event.element, event.value, "reported");
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
		serviceDdiSync();
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
		if ((-1 == currentSelectedClient) && !rows.empty())
		{
			auto preferred = std::find_if(rows.cbegin(), rows.cend(), [](const ClientRow &row) { return row.ddopActive && !row.timedOut; });
			if (preferred == rows.cend()) preferred = rows.cbegin();
			currentSelectedClient = preferred->address;
			emit selectedClientChanged();
			refreshDdop();
		}

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
		clearImplementModel();

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
			rows.push_back({ 0, "No DDOP received from this client yet." });
			rows.push_back({ 0, "It appears here automatically once the client uploads its pool," });
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
				buildImplementModel(pool);
				// Pull live values immediately so the Raw tab fills without
				// waiting for the trickle sync; quiet implements only answer.
				requestImplementDdis();
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

	void TcBridge::clearImplementModel()
	{
		currentImplementName = "No implement DDOP";
		currentImplementGeometryStatus = "Waiting for an implement object pool";
		implementElementStates.clear();
		implementDdiStates.clear();
		currentImplementElements.clear();
		currentImplementDdis.clear();
		currentTcBasicData.clear();
		nextDdiSyncIndex = 0;
		lastDdiSyncMs = 0;
		emit implementChanged();
		emit implementDdisChanged();
	}

	void TcBridge::buildImplementModel(isobus::DeviceDescriptorObjectPool &pool)
	{
		using namespace isobus::task_controller_object;
		using DDI = isobus::DataDescriptionIndex;
		int explicitGeometryCount = 0;

		for (std::uint16_t i = 0; i < pool.size(); ++i)
		{
			auto object = pool.get_object_by_index(i);
			if (auto *device = dynamic_cast<DeviceObject *>(object.get()))
			{
				currentImplementName = QString::fromStdString(device->get_designator()).trimmed();
				if (currentImplementName.isEmpty()) currentImplementName = "ISOBUS implement";
			}
			else if (auto *element = dynamic_cast<DeviceElementObject *>(object.get()))
			{
				ImplementElementState state;
				state.objectId = element->get_object_id();
				state.element = element->get_element_number();
				state.parentObjectId = element->get_parent_object();
				state.type = static_cast<int>(element->get_type());
				state.name = QString::fromStdString(element->get_designator()).trimmed();
				if (state.name.isEmpty()) state.name = QString("Element %1").arg(state.element);
				implementElementStates.push_back(state);
			}
		}

		auto geometryKindForDdi = [](std::uint16_t ddi) -> int {
			if (ddi == static_cast<std::uint16_t>(DDI::DeviceElementOffsetX)) return 1;
			if (ddi == static_cast<std::uint16_t>(DDI::DeviceElementOffsetY)) return 2;
			if (ddi == static_cast<std::uint16_t>(DDI::DeviceElementOffsetZ)) return 3;
			if ((ddi == static_cast<std::uint16_t>(DDI::PhysicalObjectWidth)) ||
			    (ddi == static_cast<std::uint16_t>(DDI::ActualWorkingWidth)) ||
			    (ddi == static_cast<std::uint16_t>(DDI::DefaultWorkingWidth)) ||
			    (ddi == static_cast<std::uint16_t>(DDI::SetpointWorkingWidth))) return 4;
			if ((ddi == static_cast<std::uint16_t>(DDI::PhysicalObjectLength)) ||
			    (ddi == static_cast<std::uint16_t>(DDI::ActualWorkingLength)) ||
			    (ddi == static_cast<std::uint16_t>(DDI::SetpointWorkingLength))) return 5;
			if (ddi == static_cast<std::uint16_t>(DDI::PhysicalObjectHeight)) return 6;
			return 0;
		};

		for (auto &elementState : implementElementStates)
		{
			auto object = pool.get_object_by_id(elementState.objectId);
			auto *element = dynamic_cast<DeviceElementObject *>(object.get());
			if (nullptr == element) continue;
			for (const auto childId : element->get_child_object_ids())
			{
				auto child = pool.get_object_by_id(childId);
				if (auto *property = dynamic_cast<DevicePropertyObject *>(child.get()))
				{
					const int kind = geometryKindForDdi(property->get_ddi());
					if (0 == kind) continue;
					const double metres = geometry_value_metres(pool, property->get_value(), property->get_device_value_presentation_object_id());
					switch (kind)
					{
						case 1: elementState.localX = metres; break;
						case 2: elementState.localY = metres; break;
						case 3: elementState.localZ = metres; break;
						case 4: elementState.width = std::abs(metres); break;
						case 5: elementState.length = std::abs(metres); break;
						case 6: elementState.height = std::abs(metres); break;
					}
					elementState.hasGeometry = true;
					++explicitGeometryCount;
				}
				else if (auto *processData = dynamic_cast<DeviceProcessDataObject *>(child.get()))
				{
					ImplementDdiState state;
					state.ddi = processData->get_ddi();
					state.element = elementState.element;
					state.triggers = processData->get_trigger_methods_bitfield();
					state.settable = processData->has_property(DeviceProcessDataObject::PropertiesBit::Settable);
					state.name = QString::fromStdString(processData->get_designator()).trimmed();
					const auto presentationId = processData->get_device_value_presentation_object_id();
					if (isobus::NULL_OBJECT_ID != presentationId)
					{
						auto presentationObject = pool.get_object_by_id(presentationId);
						if (auto *presentation = dynamic_cast<DeviceValuePresentationObject *>(presentationObject.get()))
						{
							state.displayOffset = presentation->get_offset();
							state.displayScale = presentation->get_scale();
							state.unit = QString::fromStdString(presentation->get_designator()).trimmed();
						}
					}
					state.geometryKind = geometryKindForDdi(state.ddi);
					if (0 != state.geometryKind)
					{
						state.geometryOffset = geometry_value_metres(pool, 0, processData->get_device_value_presentation_object_id());
						state.geometryScale = geometry_value_metres(pool, 1, processData->get_device_value_presentation_object_id()) - state.geometryOffset;
					}
					implementDdiStates.push_back(state);
				}
			}
		}

		// Keep the visualization useful for DDOPs that omit optional physical dimensions.
		for (auto &element : implementElementStates)
		{
			if (element.width <= 0.0) element.width = (element.type == static_cast<int>(DeviceElementObject::Type::Section)) ? 1.0 : 1.4;
			if (element.length <= 0.0) element.length = (element.type == static_cast<int>(DeviceElementObject::Type::Connector)) ? 0.35 : 0.8;
			if (element.height <= 0.0) element.height = (element.type == static_cast<int>(DeviceElementObject::Type::Bin)) ? 1.5 : 0.35;
		}
		std::vector<ImplementElementState *> sectionsWithoutGeometry;
		for (auto &element : implementElementStates)
		{
			if ((element.type == static_cast<int>(DeviceElementObject::Type::Section)) && !element.hasGeometry)
			{
				sectionsWithoutGeometry.push_back(&element);
			}
		}
		for (std::size_t i = 0; i < sectionsWithoutGeometry.size(); ++i)
		{
			sectionsWithoutGeometry[i]->localY = (static_cast<double>(i) - (static_cast<double>(sectionsWithoutGeometry.size() - 1) / 2.0)) * 1.05;
		}
		const int ddopSectionCount = static_cast<int>(std::count_if(implementElementStates.cbegin(), implementElementStates.cend(),
		  [](const ImplementElementState &element) {
			  return element.type == static_cast<int>(isobus::task_controller_object::DeviceElementObject::Type::Section);
		  }));
		const int limitedDdopSectionCount = std::clamp(ddopSectionCount, 1, 96);
		if ((ddopSectionCount > 0) && (currentSectionCount != limitedDdopSectionCount))
		{
			currentSectionCount = limitedDdopSectionCount;
			currentSectionStates = QVariantList(currentSectionCount, QVariant(false));
			emit sectionCountChanged();
			emit sectionStatesChanged();
		}

		currentImplementGeometryStatus = explicitGeometryCount > 0
		  ? QString("DDOP geometry: %1 dimensions/offsets, %2 elements").arg(explicitGeometryCount).arg(implementElementStates.size())
		  : QString("%1 DDOP elements; optional geometry is absent, using a compact fallback layout").arg(implementElementStates.size());
		publishImplementModel();
	}

	void TcBridge::publishImplementModel()
	{
		QVariantList elements;
		std::map<std::uint16_t, const ImplementElementState *> byObject;
		for (const auto &element : implementElementStates) byObject[element.objectId] = &element;
		auto absolutePosition = [&byObject](const ImplementElementState &element) {
			std::array<double, 3> position = { element.localX, element.localY, element.localZ };
			std::uint16_t parent = element.parentObjectId;
			for (int depth = 0; depth < 16; ++depth)
			{
				auto found = byObject.find(parent);
				if (found == byObject.end()) break;
				position[0] += found->second->localX;
				position[1] += found->second->localY;
				position[2] += found->second->localZ;
				parent = found->second->parentObjectId;
			}
			return position;
		};
		std::array<double, 3> connectorPosition = { 0.0, 0.0, 0.0 };
		for (const auto &element : implementElementStates)
		{
			if (element.type == static_cast<int>(isobus::task_controller_object::DeviceElementObject::Type::Connector))
			{
				connectorPosition = absolutePosition(element);
				break;
			}
		}

		for (const auto &element : implementElementStates)
		{
			const auto position = absolutePosition(element);
			QVariantMap row;
			row["objectId"] = element.objectId;
			row["element"] = element.element;
			row["name"] = element.name;
			row["type"] = element.type;
			// ISO: X longitudinal, Y lateral, Z vertical. Scene: X lateral, Z rearward.
			row["x"] = -(position[1] - connectorPosition[1]);
			row["y"] = position[2] - connectorPosition[2];
			row["z"] = -(position[0] - connectorPosition[0]);
			row["width"] = element.width;
			row["length"] = element.length;
			row["height"] = element.height;
			row["active"] = element.active;
			row["hasGeometry"] = element.hasGeometry;
			elements.push_back(row);
		}
		currentImplementElements = elements;

		QVariantList ddis;
		QVariantList basicData;
		for (const auto &state : implementDdiStates)
		{
			QVariantMap row;
			row["ddi"] = state.ddi;
			row["element"] = state.element;
			row["name"] = state.name;
			row["settable"] = state.settable;
			row["triggers"] = trigger_text(state.triggers);
			row["hasValue"] = state.hasValue;
			row["value"] = state.value;
			row["updated"] = state.updated;
			row["displayValue"] = (static_cast<double>(state.value) + state.displayOffset) * state.displayScale;
			row["unit"] = state.unit;
			ddis.push_back(row);
			const QString basicLabel = tc_basic_label(state.ddi);
			if (!basicLabel.isEmpty())
			{
				QVariantMap basic = row;
				basic["label"] = basicLabel;
				basicData.push_back(basic);
			}
		}
		currentImplementDdis = ddis;
		currentTcBasicData = basicData;

		std::vector<const ImplementElementState *> sections;
		for (const auto &element : implementElementStates)
		{
			if (element.type == static_cast<int>(isobus::task_controller_object::DeviceElementObject::Type::Section)) sections.push_back(&element);
		}
		std::sort(sections.begin(), sections.end(), [](const auto *left, const auto *right) { return left->element < right->element; });
		if (!sections.empty())
		{
			currentSectionStates.clear();
			for (const auto *section : sections) currentSectionStates.push_back(section->active);
			emit sectionStatesChanged();
		}
		emit implementChanged();
		emit implementDdisChanged();
	}

	void TcBridge::updateImplementValue(std::uint16_t ddi, std::uint16_t element, std::int32_t value)
	{
		using DDI = isobus::DataDescriptionIndex;
		bool changed = false;
		for (auto &state : implementDdiStates)
		{
			if ((state.ddi != ddi) || (state.element != element)) continue;
			state.value = value;
			state.hasValue = true;
			state.updated = timestamp_now();
			if (0 != state.geometryKind)
			{
				const double metres = state.geometryOffset + (static_cast<double>(value) * state.geometryScale);
				for (auto &geometry : implementElementStates)
				{
					if (geometry.element != element) continue;
					switch (state.geometryKind)
					{
						case 1: geometry.localX = metres; break;
						case 2: geometry.localY = metres; break;
						case 3: geometry.localZ = metres; break;
						case 4: geometry.width = std::abs(metres); break;
						case 5: geometry.length = std::abs(metres); break;
						case 6: geometry.height = std::abs(metres); break;
					}
					geometry.hasGeometry = true;
				}
			}
			changed = true;
		}

		if (ddi == static_cast<std::uint16_t>(DDI::ActualWorkState))
		{
			for (auto &geometry : implementElementStates)
			{
				if (geometry.element == element) geometry.active = ((value & 0x03) == 1);
			}
			changed = true;
		}
		const auto firstCondensed = static_cast<std::uint16_t>(DDI::ActualCondensedWorkState1_16);
		const auto lastCondensed = static_cast<std::uint16_t>(DDI::ActualCondensedWorkState241_256);
		if ((ddi >= firstCondensed) && (ddi <= lastCondensed))
		{
			std::vector<ImplementElementState *> sections;
			for (auto &geometry : implementElementStates)
			{
				if (geometry.type == static_cast<int>(isobus::task_controller_object::DeviceElementObject::Type::Section)) sections.push_back(&geometry);
			}
			std::sort(sections.begin(), sections.end(), [](const auto *left, const auto *right) { return left->element < right->element; });
			const std::size_t base = static_cast<std::size_t>(ddi - firstCondensed) * 16;
			for (std::size_t i = 0; (i < 16) && ((base + i) < sections.size()); ++i)
			{
				sections[base + i]->active = (((static_cast<std::uint32_t>(value) >> (i * 2)) & 0x03U) == 1U);
			}
			changed = true;
		}
		if (changed) publishImplementModel();
	}

	void TcBridge::setAutoDdiSync(bool enabled)
	{
		if (autoDdiSyncEnabled == enabled) return;
		autoDdiSyncEnabled = enabled;
		lastDdiSyncMs = 0;
		emit autoDdiSyncChanged();
	}

	void TcBridge::setDdiSyncIntervalMs(int intervalMs)
	{
		intervalMs = qBound(250, intervalMs, 60000);
		if (currentDdiSyncIntervalMs == intervalMs) return;
		currentDdiSyncIntervalMs = intervalMs;
		lastDdiSyncMs = 0;
		emit autoDdiSyncChanged();
	}

	void TcBridge::setLiveDdiTrafficWatch(bool enabled)
	{
		if (liveDdiTrafficWatchEnabled == enabled) return;
		liveDdiTrafficWatchEnabled = enabled;
		emit liveDdiTrafficWatchChanged();
		logs.addLine(QString("[ddi] Live traffic watch %1.").arg(enabled ? "enabled" : "disabled"));
	}

	void TcBridge::clearDdiTraffic()
	{
		ddiTraffic.clear();
	}

	void TcBridge::appendDdiTraffic(const QString &direction, const QString &command, int address, int ddi, int element,
	                                std::int32_t value, const QString &detail)
	{
		if (!liveDdiTrafficWatchEnabled) return;
		DdiTrafficRow row;
		row.timestamp = timestamp_now();
		row.direction = direction;
		row.command = command;
		row.address = address;
		row.ddi = ddi;
		row.element = element;
		row.value = value;
		row.detail = detail;
		ddiTraffic.addRow(row);
	}

	void TcBridge::requestImplementDdis()
	{
		if (!running || (nullptr == server) || (currentSelectedClient < 0) || implementDdiStates.empty()) return;
		auto client = server->find_client(static_cast<std::uint8_t>(currentSelectedClient));
		if (nullptr == client) return;
		int sentCount = 0;
		for (const auto &state : implementDdiStates)
		{
			if (server->send_request_value(client, state.ddi, state.element))
			{
				++sentCount;
				appendDdiTraffic("TC -> client", "Request", currentSelectedClient, state.ddi, state.element, 0, state.name);
			}
		}
		logs.addLine(QString("[ddop] Requested %1/%2 declared process values from %3.")
		               .arg(sentCount).arg(implementDdiStates.size()).arg(currentImplementName));
		lastDdiSyncMs = steady_clock_ms();
	}

	void TcBridge::serviceDdiSync()
	{
		if (!autoDdiSyncEnabled || implementDdiStates.empty() || (currentSelectedClient < 0)) return;
		const auto now = steady_clock_ms();
		if ((lastDdiSyncMs != 0) && ((now - lastDdiSyncMs) < static_cast<std::uint64_t>(currentDdiSyncIntervalMs))) return;
		auto client = server->find_client(static_cast<std::uint8_t>(currentSelectedClient));
		if (nullptr == client) return;

		using Trigger = isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods;
		const std::size_t batch = std::min<std::size_t>(4, implementDdiStates.size());
		for (std::size_t count = 0; count < batch; ++count)
		{
			auto &state = implementDdiStates[nextDdiSyncIndex % implementDdiStates.size()];
			if (!state.reportingConfigured && (state.triggers & static_cast<std::uint8_t>(Trigger::TimeInterval)))
			{
				state.reportingConfigured = server->send_time_interval_measurement_command(
				  client, state.ddi, state.element, static_cast<std::uint32_t>(currentDdiSyncIntervalMs));
				if (state.reportingConfigured)
				{
					appendDdiTraffic("TC -> client", "Time interval", currentSelectedClient, state.ddi, state.element,
					                 currentDdiSyncIntervalMs, state.name);
				}
			}
			if (server->send_request_value(client, state.ddi, state.element))
			{
				appendDdiTraffic("TC -> client", "Request", currentSelectedClient, state.ddi, state.element, 0, state.name);
			}
			++nextDdiSyncIndex;
		}
		lastDdiSyncMs = now;
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
		if (active)
		{
			startSelectedTask();
		}
		else
		{
			stopSelectedTask();
		}
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
		if (sent) appendDdiTraffic("TC -> client", "Request", currentSelectedClient, ddi, element, 0, "manual");
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
		if (sent) appendDdiTraffic("TC -> client", acknowledge ? "Set+Ack" : "Set", currentSelectedClient, ddi, element, value, "manual");
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
		if (sent) appendDdiTraffic("TC -> client", QString("Measurement %1").arg(kind), currentSelectedClient, ddi, element, value, "manual");
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
		count = qBound(1, count, 96);
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

	bool TcBridge::startGps(const QString &source, const QString &serialPort, int baudRate,
	                        double latitude, double longitude)
	{
		gpsProvider.stop();
		currentGpsSourceText = source;
		if (source.compare("Simulated", Qt::CaseInsensitive) == 0)
		{
			gpsProvider.configure_simulation(latitude, longitude, 0.0, 0.0);
		}
		else
		{
			GpsSource gpsSource = GpsSource::Auto;
			if (source.compare("NMEA serial", Qt::CaseInsensitive) == 0)
			{
				if (serialPort.trimmed().isEmpty())
				{
					setStatus("Enter a serial port for NMEA GPS (for example COM4). ");
					return false;
				}
				gpsSource = GpsSource::NmeaOnly;
			}
			else if (source.compare("ISO CAN", Qt::CaseInsensitive) == 0)
			{
				gpsSource = GpsSource::IsoOnly;
			}
			gpsProvider.set_source(gpsSource);
			if (!gpsProvider.start(serialPort.trimmed().toStdString(), static_cast<std::uint32_t>(qBound(1200, baudRate, 921600))))
			{
				setStatus("GPS source could not be started.");
				return false;
			}
		}
		gpsRunningFlag = true;
		lastMotionUpdateMs = 0;
		currentMachineDistanceMm = 0;
		trailerPoseValid = false;
		setStatus(QString("GPS started: %1.").arg(source));
		logs.addLine(QString("[gps] Source started: %1.").arg(source));
		emit gpsChanged();
		return true;
	}

	void TcBridge::stopGps()
	{
		if (!gpsRunningFlag)
		{
			return;
		}
		gpsProvider.stop();
		gpsRunningFlag = false;
		currentGps.valid = false;
		currentGpsSourceText = "Off";
		lastMotionUpdateMs = 0;
		logs.addLine("[gps] Source stopped.");
		emit gpsChanged();
	}

	void TcBridge::setSimulationMotion(double speedKph, double courseDeg)
	{
		if (!gpsRunningFlag || (currentGpsSourceText != "Simulated"))
		{
			setStatus("Start simulated GPS before changing its motion.");
			return;
		}
		currentThrottleKph = std::clamp(speedKph, 0.0, 50.0);
		currentSteeringAngle = 0.0;
		gpsProvider.set_simulation_motion(currentThrottleKph / 3.6, courseDeg);
		emit drivingControlsChanged();
	}

	void TcBridge::nudgeSimulation(double forwardMeters, double turnDegrees)
	{
		if (!gpsRunningFlag || (currentGpsSourceText != "Simulated"))
		{
			setStatus("Start simulated GPS before moving the tractor.");
			return;
		}
		gpsProvider.nudge_simulation(forwardMeters, turnDegrees);
		updateGps();
	}

	bool TcBridge::createField(const QString &name, double widthM, double lengthM)
	{
		if (!currentGps.valid || !currentGps.latitudeDeg || !currentGps.longitudeDeg)
		{
			setStatus("Start GPS and wait for a valid position before defining a field.");
			return false;
		}
		if (name.trimmed().isEmpty())
		{
			setStatus("Enter a field name.");
			return false;
		}
		widthM = std::clamp(widthM, 1.0, 100000.0);
		lengthM = std::clamp(lengthM, 1.0, 100000.0);
		const double latitude = *currentGps.latitudeDeg;
		const double longitude = *currentGps.longitudeDeg;
		const double latitudeOffset = (lengthM * 0.5 / EarthRadiusM) / DegreesToRadians;
		const double longitudeScale = std::max(0.01, std::cos(latitude * DegreesToRadians));
		const double longitudeOffset = (widthM * 0.5 / (EarthRadiusM * longitudeScale)) / DegreesToRadians;

		FieldBoundary field;
		field.name = name.trimmed().toStdString();
		field.exteriorRing = {
			{ latitude - latitudeOffset, longitude - longitudeOffset },
			{ latitude - latitudeOffset, longitude + longitudeOffset },
			{ latitude + latitudeOffset, longitude + longitudeOffset },
			{ latitude + latitudeOffset, longitude - longitudeOffset },
			{ latitude - latitudeOffset, longitude - longitudeOffset }
		};
		const std::string fieldId = fieldTaskManager.add_field(field);
		if (fieldId.empty())
		{
			setStatus("Field could not be created.");
			return false;
		}
		refreshFieldNames();
		const auto selected = std::find(fieldIds.begin(), fieldIds.end(), fieldId);
		selectField(static_cast<int>(std::distance(fieldIds.begin(), selected)));
		clearTrack();
		setStatus(QString("Field '%1' created at the current GPS position.").arg(name.trimmed()));
		logs.addLine(QString("[field] Created %1 (%2 m x %3 m).").arg(name.trimmed()).arg(widthM, 0, 'f', 1).arg(lengthM, 0, 'f', 1));
		return true;
	}

	void TcBridge::selectField(int index)
	{
		if ((index < 0) || (index >= static_cast<int>(fieldIds.size())))
		{
			return;
		}
		currentSelectedField = index;
		updateFieldSelection();
		emit fieldsChanged();
	}

	bool TcBridge::createTask(const QString &name)
	{
		if ((currentSelectedField < 0) || (currentSelectedField >= static_cast<int>(fieldIds.size())))
		{
			setStatus("Create or select a field before creating a task.");
			return false;
		}
		if (name.trimmed().isEmpty())
		{
			setStatus("Enter a task name.");
			return false;
		}
		Task task;
		task.name = name.trimmed().toStdString();
		task.fieldId = fieldIds[static_cast<std::size_t>(currentSelectedField)];
		if (currentSelectedClient >= 0)
		{
			task.clientId = std::to_string(currentSelectedClient);
		}
		const std::string taskId = fieldTaskManager.create_task(task);
		if (taskId.empty())
		{
			setStatus("Task could not be created.");
			return false;
		}
		refreshTaskNames();
		const auto selected = std::find(taskIds.begin(), taskIds.end(), taskId);
		selectTask(static_cast<int>(std::distance(taskIds.begin(), selected)));
		setStatus(QString("Task '%1' created for field '%2'.").arg(name.trimmed(), currentActiveFieldName));
		logs.addLine(QString("[task] Created %1 for %2.").arg(name.trimmed(), currentActiveFieldName));
		return true;
	}

	void TcBridge::selectTask(int index)
	{
		if ((index < 0) || (index >= static_cast<int>(taskIds.size())))
		{
			return;
		}
		currentSelectedTask = index;
		const auto task = fieldTaskManager.get_task(taskIds[static_cast<std::size_t>(index)]);
		if (task)
		{
			const auto field = std::find(fieldIds.begin(), fieldIds.end(), task->fieldId);
			if (field != fieldIds.end())
			{
				currentSelectedField = static_cast<int>(std::distance(fieldIds.begin(), field));
				updateFieldSelection();
				emit fieldsChanged();
			}
		}
		emit tasksChanged();
	}

	void TcBridge::startSelectedTask()
	{
		if ((currentSelectedTask < 0) || (currentSelectedTask >= static_cast<int>(taskIds.size())))
		{
			setStatus("Create or select a task first.");
			return;
		}
		const std::string &taskId = taskIds[static_cast<std::size_t>(currentSelectedTask)];
		if (!fieldTaskManager.start_task(taskId))
		{
			setStatus("Task could not be started.");
			return;
		}
		taskActive = true;
		currentActiveTaskName = currentTaskNames.at(currentSelectedTask);
		if (nullptr != server)
		{
			server->set_task_totals_active(true);
		}
		emit taskActiveChanged();
		emit tasksChanged();
		setStatus(QString("Task '%1' is active.").arg(currentActiveTaskName));
		logs.addLine(QString("[task] Started %1.").arg(currentActiveTaskName));
		// Many implements only report process data while a task is active.
		requestImplementDdis();
	}

	void TcBridge::pauseSelectedTask()
	{
		if ((currentSelectedTask < 0) || (currentSelectedTask >= static_cast<int>(taskIds.size())))
		{
			return;
		}
		if (fieldTaskManager.pause_task(taskIds[static_cast<std::size_t>(currentSelectedTask)]))
		{
			taskActive = false;
			if (nullptr != server)
			{
				server->set_task_totals_active(false);
			}
			emit taskActiveChanged();
			emit tasksChanged();
			setStatus(QString("Task '%1' paused.").arg(currentActiveTaskName));
			logs.addLine(QString("[task] Paused %1.").arg(currentActiveTaskName));
		}
	}

	void TcBridge::stopSelectedTask()
	{
		if ((currentSelectedTask < 0) || (currentSelectedTask >= static_cast<int>(taskIds.size())))
		{
			return;
		}
		const QString stoppedName = currentTaskNames.at(currentSelectedTask);
		if (fieldTaskManager.stop_task(taskIds[static_cast<std::size_t>(currentSelectedTask)]))
		{
			taskActive = false;
			currentActiveTaskName.clear();
			if (nullptr != server)
			{
				server->set_task_totals_active(false);
			}
			emit taskActiveChanged();
			emit tasksChanged();
			setStatus(QString("Task '%1' completed.").arg(stoppedName));
			logs.addLine(QString("[task] Completed %1.").arg(stoppedName));
		}
	}

	void TcBridge::clearTrack()
	{
		currentTrackPoints.clear();
		emit trackChanged();
	}

	namespace
	{
		QJsonArray encodeRing(const std::vector<std::pair<double, double>> &ring)
		{
			QJsonArray out;
			for (const auto &[latitude, longitude] : ring)
			{
				QJsonArray point;
				point.push_back(latitude);
				point.push_back(longitude);
				out.push_back(point);
			}
			return out;
		}

		std::vector<std::pair<double, double>> decodeRing(const QJsonArray &ring)
		{
			std::vector<std::pair<double, double>> out;
			for (const auto &entry : ring)
			{
				const auto point = entry.toArray();
				if (point.size() >= 2)
				{
					out.emplace_back(point.at(0).toDouble(), point.at(1).toDouble());
				}
			}
			return out;
		}
	} // namespace

	void TcBridge::saveFields(const QUrl &fileUrl)
	{
		QJsonArray fields;
		for (const auto &field : fieldTaskManager.list_fields())
		{
			QJsonObject object;
			object["id"] = QString::fromStdString(field.id);
			object["name"] = QString::fromStdString(field.name);
			object["createdMs"] = static_cast<qint64>(field.createdMs);
			object["areaHectares"] = field.areaHectares;
			object["exteriorRing"] = encodeRing(field.exteriorRing);
			QJsonArray holes;
			for (const auto &hole : field.interiorRings)
			{
				holes.push_back(encodeRing(hole));
			}
			object["interiorRings"] = holes;
			fields.push_back(object);
		}
		QJsonDocument document(QJsonObject{ { "fields", fields } });
		QFile file(fileUrl.toLocalFile());
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			setStatus("Could not write field file.");
			return;
		}
		file.write(document.toJson(QJsonDocument::Indented));
		setStatus(QString("Saved %1 field(s).").arg(fields.size()));
		logs.addLine(QString("[field] Saved %1 field(s) to %2.").arg(fields.size()).arg(fileUrl.fileName()));
	}

	void TcBridge::loadFields(const QUrl &fileUrl)
	{
		QFile file(fileUrl.toLocalFile());
		if (!file.open(QIODevice::ReadOnly))
		{
			setStatus("Could not open field file.");
			return;
		}
		const auto document = QJsonDocument::fromJson(file.readAll());
		if (!document.isObject())
		{
			setStatus("Field file is not valid JSON.");
			return;
		}
		int loaded = 0;
		for (const auto &entry : document.object().value("fields").toArray())
		{
			const auto object = entry.toObject();
			FieldBoundary field;
			field.id = object.value("id").toString().toStdString();
			field.name = object.value("name").toString().toStdString();
			field.createdMs = static_cast<std::uint64_t>(object.value("createdMs").toVariant().toLongLong());
			field.areaHectares = object.value("areaHectares").toDouble();
			field.exteriorRing = decodeRing(object.value("exteriorRing").toArray());
			for (const auto &hole : object.value("interiorRings").toArray())
			{
				field.interiorRings.push_back(decodeRing(hole.toArray()));
			}
			if (field.exteriorRing.size() >= 3)
			{
				fieldTaskManager.add_field(field);
				++loaded;
			}
		}
		refreshFieldNames();
		rebuildFieldBoundaryPoints();
		setStatus(QString("Loaded %1 field(s).").arg(loaded));
		logs.addLine(QString("[field] Loaded %1 field(s) from %2.").arg(loaded).arg(fileUrl.fileName()));
	}

	void TcBridge::saveTasks(const QUrl &fileUrl)
	{
		QJsonArray tasks;
		for (const auto &task : fieldTaskManager.list_tasks())
		{
			QJsonObject object;
			object["id"] = QString::fromStdString(task.id);
			object["name"] = QString::fromStdString(task.name);
			object["fieldId"] = QString::fromStdString(task.fieldId);
			object["clientId"] = QString::fromStdString(task.clientId);
			object["createdMs"] = static_cast<qint64>(task.createdMs);
			object["startedMs"] = static_cast<qint64>(task.startedMs);
			object["stoppedMs"] = static_cast<qint64>(task.stoppedMs);
			object["state"] = static_cast<int>(task.state);
			object["logIntervalMs"] = static_cast<qint64>(task.logIntervalMs);
			QJsonArray ddis;
			for (const auto ddi : task.ddIsToLog)
			{
				ddis.push_back(static_cast<int>(ddi));
			}
			object["ddIsToLog"] = ddis;
			tasks.push_back(object);
		}
		QJsonDocument document(QJsonObject{ { "tasks", tasks } });
		QFile file(fileUrl.toLocalFile());
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			setStatus("Could not write task file.");
			return;
		}
		file.write(document.toJson(QJsonDocument::Indented));
		setStatus(QString("Saved %1 task(s).").arg(tasks.size()));
		logs.addLine(QString("[task] Saved %1 task(s) to %2.").arg(tasks.size()).arg(fileUrl.fileName()));
	}

	void TcBridge::loadTasks(const QUrl &fileUrl)
	{
		QFile file(fileUrl.toLocalFile());
		if (!file.open(QIODevice::ReadOnly))
		{
			setStatus("Could not open task file.");
			return;
		}
		const auto document = QJsonDocument::fromJson(file.readAll());
		if (!document.isObject())
		{
			setStatus("Task file is not valid JSON.");
			return;
		}
		int loaded = 0;
		int skipped = 0;
		for (const auto &entry : document.object().value("tasks").toArray())
		{
			const auto object = entry.toObject();
			Task task;
			task.id = object.value("id").toString().toStdString();
			task.name = object.value("name").toString().toStdString();
			task.fieldId = object.value("fieldId").toString().toStdString();
			task.clientId = object.value("clientId").toString().toStdString();
			task.createdMs = static_cast<std::uint64_t>(object.value("createdMs").toVariant().toLongLong());
			task.startedMs = static_cast<std::uint64_t>(object.value("startedMs").toVariant().toLongLong());
			task.stoppedMs = static_cast<std::uint64_t>(object.value("stoppedMs").toVariant().toLongLong());
			task.logIntervalMs = static_cast<std::uint32_t>(object.value("logIntervalMs").toVariant().toLongLong());
			for (const auto ddi : object.value("ddIsToLog").toArray())
			{
				task.ddIsToLog.push_back(static_cast<std::uint16_t>(ddi.toInt()));
			}
			// Never restore a live state; loaded tasks always start as created.
			task.state = Task::State::Created;
			if (!task.name.empty() && !fieldTaskManager.create_task(task).empty())
			{
				++loaded;
			}
			else
			{
				// Usually a task whose field has not been loaded (yet).
				++skipped;
			}
		}
		refreshTaskNames();
		setStatus(skipped > 0
		            ? QString("Loaded %1 task(s), %2 skipped (load their fields first).").arg(loaded).arg(skipped)
		            : QString("Loaded %1 task(s).").arg(loaded));
		logs.addLine(QString("[task] Loaded %1 task(s)%2 from %3.")
		               .arg(loaded)
		               .arg(skipped > 0 ? QString(", %1 skipped (missing field)").arg(skipped) : QString())
		               .arg(fileUrl.fileName()));
	}

	bool TcBridge::startBoundaryRecording(const QString &name)
	{
		if (!currentGps.valid || !currentGps.latitudeDeg || !currentGps.longitudeDeg)
		{
			setStatus("Start GPS and wait for a valid position before recording a boundary.");
			return false;
		}
		if (name.trimmed().isEmpty())
		{
			setStatus("Enter a field name before recording its boundary.");
			return false;
		}
		recordedBoundaryName = name.trimmed();
		recordedBoundary.clear();
		recordedBoundary.emplace_back(*currentGps.latitudeDeg, *currentGps.longitudeDeg);
		boundaryRecordingFlag = true;
		fieldOriginLatitude = *currentGps.latitudeDeg;
		fieldOriginLongitude = *currentGps.longitudeDeg;
		fieldOriginValid = true;
		rebuildFieldBoundaryPoints();
		emit boundaryChanged();
		setStatus(QString("Recording perimeter for '%1'. Drive around the field and finish near the start.").arg(recordedBoundaryName));
		return true;
	}

	bool TcBridge::finishBoundaryRecording()
	{
		if (!boundaryRecordingFlag || (recordedBoundary.size() < 3))
		{
			setStatus("At least three perimeter points are required.");
			return false;
		}
		if (recordedBoundary.front() != recordedBoundary.back()) recordedBoundary.push_back(recordedBoundary.front());
		FieldBoundary field;
		field.name = recordedBoundaryName.toStdString();
		field.exteriorRing = recordedBoundary;
		const auto fieldId = fieldTaskManager.add_field(field);
		if (fieldId.empty())
		{
			setStatus("The recorded field boundary could not be saved.");
			return false;
		}
		boundaryRecordingFlag = false;
		refreshFieldNames();
		const auto selected = std::find(fieldIds.begin(), fieldIds.end(), fieldId);
		selectField(static_cast<int>(std::distance(fieldIds.begin(), selected)));
		recordedBoundary.clear();
		emit boundaryChanged();
		setStatus(QString("Field '%1' saved from %2 perimeter points.").arg(recordedBoundaryName).arg(field.exteriorRing.size() - 1));
		logs.addLine(QString("[field] Recorded perimeter for %1.").arg(recordedBoundaryName));
		return true;
	}

	void TcBridge::cancelBoundaryRecording()
	{
		boundaryRecordingFlag = false;
		recordedBoundary.clear();
		rebuildFieldBoundaryPoints();
		emit boundaryChanged();
		setStatus("Field perimeter recording cancelled.");
	}

	bool TcBridge::createFieldFromLocalBoundary(const QString &name, const QVariantList &points)
	{
		if (name.trimmed().isEmpty())
		{
			setStatus("Enter a field name before creating a drawn field.");
			return false;
		}
		if (points.size() < 3)
		{
			setStatus("Draw at least three boundary points before creating a field.");
			return false;
		}
		if (!fieldOriginValid)
		{
			if (currentGps.valid && currentGps.latitudeDeg && currentGps.longitudeDeg)
			{
				fieldOriginLatitude = *currentGps.latitudeDeg;
				fieldOriginLongitude = *currentGps.longitudeDeg;
			}
			else
			{
				fieldOriginLatitude = 52.0;
				fieldOriginLongitude = 5.0;
				logs.addLine("[field] No GPS origin was available; drawn field saved around the default origin 52.0, 5.0.");
			}
			fieldOriginValid = true;
		}

		FieldBoundary field;
		field.name = name.trimmed().toStdString();
		field.exteriorRing.reserve(static_cast<std::size_t>(points.size() + 1));
		for (const auto &variant : points)
		{
			const QVariantMap point = variant.toMap();
			const double x = point.value("x").toDouble();
			const double z = point.value("z").toDouble();
			const double latitude = fieldOriginLatitude - ((z / EarthRadiusM) / DegreesToRadians);
			const double longitudeScale = std::max(0.01, std::cos(fieldOriginLatitude * DegreesToRadians));
			const double longitude = fieldOriginLongitude + ((x / (EarthRadiusM * longitudeScale)) / DegreesToRadians);
			field.exteriorRing.emplace_back(latitude, longitude);
		}
		if (field.exteriorRing.front() != field.exteriorRing.back())
		{
			field.exteriorRing.push_back(field.exteriorRing.front());
		}
		const auto fieldId = fieldTaskManager.add_field(field);
		if (fieldId.empty())
		{
			setStatus("The drawn field boundary could not be saved.");
			return false;
		}
		boundaryRecordingFlag = false;
		recordedBoundary.clear();
		refreshFieldNames();
		const auto selected = std::find(fieldIds.begin(), fieldIds.end(), fieldId);
		selectField(static_cast<int>(std::distance(fieldIds.begin(), selected)));
		setStatus(QString("Drawn field '%1' saved with %2 boundary points.").arg(name.trimmed()).arg(points.size()));
		logs.addLine(QString("[field] Created drawn field %1 from %2 points.").arg(name.trimmed()).arg(points.size()));
		return true;
	}

	void TcBridge::setSteeringAngle(double degrees)
	{
		degrees = std::clamp(degrees, -40.0, 40.0);
		if (std::abs(currentSteeringAngle - degrees) < 0.05) return;
		currentSteeringAngle = degrees;
		emit drivingControlsChanged();
	}

	void TcBridge::setThrottleKph(double speedKph)
	{
		speedKph = std::clamp(speedKph, 0.0, 50.0);
		if (std::abs(currentThrottleKph - speedKph) < 0.01) return;
		currentThrottleKph = speedKph;
		if (gpsRunningFlag && (currentGpsSourceText == "Simulated"))
		{
			gpsProvider.set_simulation_motion(currentThrottleKph / 3.6, currentGps.courseDeg.value_or(0.0));
		}
		emit drivingControlsChanged();
	}

	void TcBridge::adjustThrottle(double deltaKph) { setThrottleKph(currentThrottleKph + deltaKph); }

	void TcBridge::stopTractor()
	{
		setThrottleKph(0.0);
		setStatus("Tractor stopped.");
	}

	void TcBridge::clearWorkedArea()
	{
		currentWorkedPoints.clear();
		currentWorkedAreaHa = 0.0;
		currentWorkedDistanceM = 0.0;
		currentWorkedTimeSeconds = 0.0;
		coveragePositionValid = false;
		emit workChanged();
	}

	void TcBridge::rebuildFieldBoundaryPoints()
	{
		currentFieldBoundaryPoints.clear();
		std::vector<std::pair<double, double>> points;
		if (boundaryRecordingFlag)
		{
			points = recordedBoundary;
		}
		else if ((currentSelectedField >= 0) && (currentSelectedField < static_cast<int>(fieldIds.size())))
		{
			const auto field = fieldTaskManager.get_field(fieldIds[static_cast<std::size_t>(currentSelectedField)]);
			if (field) points = field->exteriorRing;
		}
		if (!fieldOriginValid) return;
		for (const auto &[latitude, longitude] : points)
		{
			QVariantMap point;
			point["x"] = (longitude - fieldOriginLongitude) * DegreesToRadians * EarthRadiusM * std::cos(fieldOriginLatitude * DegreesToRadians);
			point["z"] = -(latitude - fieldOriginLatitude) * DegreesToRadians * EarthRadiusM;
			currentFieldBoundaryPoints.push_back(point);
		}
		emit boundaryChanged();
	}

	void TcBridge::updateTrailerPose(double elapsedSeconds)
	{
		const double tractorCourse = currentGps.courseDeg.value_or(0.0);
		if (!trailerPoseValid)
		{
			currentImplementCourse = tractorCourse;
			trailerPoseValid = true;
		}
		const double difference = std::remainder(tractorCourse - currentImplementCourse, 360.0);
		const double speedMps = currentGps.speedMps.value_or(0.0);
		const double yawRateDeg = (speedMps / 4.5) * std::sin(difference * DegreesToRadians) / DegreesToRadians;
		currentImplementCourse = std::fmod(currentImplementCourse + (yawRateDeg * elapsedSeconds) + 360.0, 360.0);
		const double heading = tractorCourse * DegreesToRadians;
		currentImplementX = currentTractorX - (std::sin(heading) * 2.8);
		currentImplementZ = currentTractorZ + (std::cos(heading) * 2.8);
	}

	void TcBridge::updateWorkCoverage(double elapsedSeconds)
	{
		const double distance = coveragePositionValid ? std::hypot(currentImplementX - lastCoverageX, currentImplementZ - lastCoverageZ) : 0.0;
		lastCoverageX = currentImplementX;
		lastCoverageZ = currentImplementZ;
		coveragePositionValid = true;
		// Accrue when our task is active, or whenever the client itself reports
		// sections ON (covers implements worked without an app-side task).
		if ((!taskActive && (activeSectionCount() == 0)) || (distance > 10.0)) return;
		double activeWidth = 0.0;
		for (const auto &element : implementElementStates)
		{
			if ((element.type == static_cast<int>(isobus::task_controller_object::DeviceElementObject::Type::Section)) && element.active)
			{
				activeWidth += element.width;
			}
		}
		if (activeWidth <= 0.0) activeWidth = static_cast<double>(activeSectionCount());
		currentWorkedDistanceM += distance;
		currentWorkedAreaHa += (distance * activeWidth) / 10000.0;
		currentWorkedTimeSeconds += elapsedSeconds;
		bool appendCoverage = currentWorkedPoints.isEmpty();
		if (!appendCoverage)
		{
			const QVariantMap last = currentWorkedPoints.constLast().toMap();
			appendCoverage = std::hypot(currentImplementX - last.value("x").toDouble(),
			                                currentImplementZ - last.value("z").toDouble()) >= 0.4;
		}
		if (appendCoverage && (currentWorkedPoints.size() < 3000))
		{
			QVariantMap point;
			point["x"] = currentImplementX;
			point["z"] = currentImplementZ;
			point["width"] = activeWidth;
			point["course"] = currentImplementCourse;
			currentWorkedPoints.push_back(point);
		}
		emit workChanged();
	}

	void TcBridge::updateSpeedMessages(double elapsedSeconds)
	{
		if (nullptr == speedMessages)
		{
			return;
		}
		static constexpr double MAX_SPEED_MM_PER_SEC = 64255.0;

		// Publish our sensed motion (GPS, driven by real receiver or simulator)
		// as ground-based, wheel-based, and machine-selected speed so that
		// implements and terminals on the bus pick up speed and distance.
		std::uint16_t speedMmPerSec = 0;
		auto direction = isobus::SpeedMessagesInterface::MachineDirection::NotAvailable;
		auto selectedSpeedSource = isobus::SpeedMessagesInterface::MachineSelectedSpeedData::SpeedSource::NotAvailable;
		if (currentGps.valid && currentGps.speedMps.has_value())
		{
			const double speedMps = std::max(0.0, *currentGps.speedMps);
			speedMmPerSec = static_cast<std::uint16_t>(std::min(MAX_SPEED_MM_PER_SEC, speedMps * 1000.0));
			direction = isobus::SpeedMessagesInterface::MachineDirection::Forward;
			selectedSpeedSource = (currentGpsSourceText == "Simulated") ?
			                        isobus::SpeedMessagesInterface::MachineSelectedSpeedData::SpeedSource::Simulated :
			                        isobus::SpeedMessagesInterface::MachineSelectedSpeedData::SpeedSource::NavigationBasedSpeed;
			currentMachineDistanceMm += static_cast<std::uint32_t>(std::max(0.0, speedMps * 1000.0 * elapsedSeconds));
		}

		speedMessages->groundBasedSpeedTransmitData.set_machine_speed(speedMmPerSec);
		speedMessages->groundBasedSpeedTransmitData.set_machine_distance(currentMachineDistanceMm);
		speedMessages->groundBasedSpeedTransmitData.set_machine_direction_of_travel(direction);
		speedMessages->wheelBasedSpeedTransmitData.set_machine_speed(speedMmPerSec);
		speedMessages->wheelBasedSpeedTransmitData.set_machine_distance(currentMachineDistanceMm);
		speedMessages->wheelBasedSpeedTransmitData.set_machine_direction_of_travel(direction);
		speedMessages->wheelBasedSpeedTransmitData.set_key_switch_state(
		  isobus::SpeedMessagesInterface::WheelBasedMachineSpeedData::KeySwitchState::NotOff);
		speedMessages->wheelBasedSpeedTransmitData.set_implement_start_stop_operations_state(
		  taskActive ?
		    isobus::SpeedMessagesInterface::WheelBasedMachineSpeedData::ImplementStartStopOperations::StartEnableImplementOperations :
		    isobus::SpeedMessagesInterface::WheelBasedMachineSpeedData::ImplementStartStopOperations::StopDisableImplementOperations);
		speedMessages->wheelBasedSpeedTransmitData.set_operator_direction_reversed_state(
		  isobus::SpeedMessagesInterface::WheelBasedMachineSpeedData::OperatorDirectionReversed::NotReversed);
		speedMessages->machineSelectedSpeedTransmitData.set_machine_speed(speedMmPerSec);
		speedMessages->machineSelectedSpeedTransmitData.set_machine_distance(currentMachineDistanceMm);
		speedMessages->machineSelectedSpeedTransmitData.set_machine_direction_of_travel(direction);
		speedMessages->machineSelectedSpeedTransmitData.set_exit_reason_code(
		  static_cast<std::uint8_t>(isobus::SpeedMessagesInterface::MachineSelectedSpeedData::ExitReasonCode::NoReasonAllClear));
		speedMessages->machineSelectedSpeedTransmitData.set_speed_source(selectedSpeedSource);
		speedMessages->machineSelectedSpeedTransmitData.set_limit_status(
		  isobus::SpeedMessagesInterface::MachineSelectedSpeedData::LimitStatus::NotLimited);
		speedMessages->update();
	}

	void TcBridge::updateGps()
	{
		if (!gpsRunningFlag)
		{
			return;
		}
		const auto nowMs = steady_clock_ms();
		const double elapsedSeconds = (lastMotionUpdateMs == 0) ? 0.0 : std::min(0.5, static_cast<double>(nowMs - lastMotionUpdateMs) / 1000.0);
		lastMotionUpdateMs = nowMs;
		if ((currentGpsSourceText == "Simulated") && (elapsedSeconds > 0.0))
		{
			const double currentCourse = currentGps.courseDeg.value_or(0.0);
			const double yawRate = (currentThrottleKph / 3.6 / 3.0) * std::tan(currentSteeringAngle * DegreesToRadians);
			const double nextCourse = currentCourse + ((yawRate * elapsedSeconds) / DegreesToRadians);
			gpsProvider.set_simulation_motion(currentThrottleKph / 3.6, nextCourse);
		}
		gpsProvider.update();
		const GpsSolution solution = gpsProvider.current_solution();
		if (!solution.valid || !solution.latitudeDeg || !solution.longitudeDeg)
		{
			if (currentGps.valid)
			{
				currentGps = solution;
				emit gpsChanged();
			}
			updateSpeedMessages(elapsedSeconds);
			return;
		}
		currentGps = solution;
		if (!fieldOriginValid)
		{
			fieldOriginLatitude = *solution.latitudeDeg;
			fieldOriginLongitude = *solution.longitudeDeg;
			fieldOriginValid = true;
		}
		currentTractorX = (*solution.longitudeDeg - fieldOriginLongitude) * DegreesToRadians * EarthRadiusM *
		                  std::cos(fieldOriginLatitude * DegreesToRadians);
		const double north = (*solution.latitudeDeg - fieldOriginLatitude) * DegreesToRadians * EarthRadiusM;
		currentTractorZ = -north;
		updateTrailerPose(elapsedSeconds);

		bool appendPoint = currentTrackPoints.isEmpty();
		if (!appendPoint)
		{
			const QVariantMap last = currentTrackPoints.constLast().toMap();
			appendPoint = std::hypot(currentTractorX - last.value("x").toDouble(),
			                         currentTractorZ - last.value("z").toDouble()) >= 0.75;
		}
		if (appendPoint)
		{
			QVariantMap point;
			point.insert("x", currentTractorX);
			point.insert("z", currentTractorZ);
			currentTrackPoints.push_back(point);
			if (currentTrackPoints.size() > 800)
			{
				currentTrackPoints.removeFirst();
			}
			emit trackChanged();
		}
		if (boundaryRecordingFlag)
		{
			bool appendBoundary = recordedBoundary.empty();
			if (!appendBoundary)
			{
				const auto &[lastLatitude, lastLongitude] = recordedBoundary.back();
				const double dx = (*solution.longitudeDeg - lastLongitude) * DegreesToRadians * EarthRadiusM *
				                  std::cos(*solution.latitudeDeg * DegreesToRadians);
				const double dz = (*solution.latitudeDeg - lastLatitude) * DegreesToRadians * EarthRadiusM;
				appendBoundary = std::hypot(dx, dz) >= 1.0;
			}
			if (appendBoundary)
			{
				recordedBoundary.emplace_back(*solution.latitudeDeg, *solution.longitudeDeg);
				rebuildFieldBoundaryPoints();
			}
		}
		updateWorkCoverage(elapsedSeconds);
		fieldTaskManager.on_position_update(solution);
		emit gpsChanged();
		updateSpeedMessages(elapsedSeconds);
		updateNmea2000Gps();
	}

	void TcBridge::updateNmea2000Gps()
	{
		if (nullptr == nmea2000)
		{
			return;
		}
		if (currentGps.valid && currentGps.latitudeDeg.has_value() && currentGps.longitudeDeg.has_value())
		{
			const double latitude = *currentGps.latitudeDeg;
			const double longitude = *currentGps.longitudeDeg;
			const double speedMps = (currentGps.speedMps.has_value()) ? std::max(0.0, *currentGps.speedMps) : 0.0;
			const bool haveCourse = currentGps.courseDeg.has_value();
			const double courseRad = haveCourse ? (*currentGps.courseDeg * DegreesToRadians) : 0.0;

			// PGN 129025 position rapid update (1e-7 degrees).
			auto &position = nmea2000->get_position_rapid_update_transmit_message();
			position.set_latitude(static_cast<std::int32_t>(latitude * 1e7));
			position.set_longitude(static_cast<std::int32_t>(longitude * 1e7));

			// PGN 129026 COG/SOG rapid update (1e-4 rad, 0.01 m/s).
			auto &cogSog = nmea2000->get_cog_sog_transmit_message();
			cogSog.set_course_over_ground(haveCourse ? static_cast<std::uint16_t>(std::fmod(courseRad, 2.0 * 3.14159265358979323846) * 1e4) : 0xFFFF);
			cogSog.set_speed_over_ground(static_cast<std::uint16_t>(std::min<double>(0xFFFE, speedMps * 100.0)));
			cogSog.set_course_over_ground_reference(isobus::NMEA2000Messages::CourseOverGroundSpeedOverGroundRapidUpdate::CourseOverGroundReference::True);

			// PGN 129029 GNSS position data (1e-16 degrees, 1e-6 m altitude).
			auto &gnss = nmea2000->get_gnss_position_data_transmit_message();
			gnss.set_latitude(static_cast<std::int64_t>(latitude * 1e16));
			gnss.set_longitude(static_cast<std::int64_t>(longitude * 1e16));
			if (currentGps.altitudeM.has_value())
			{
				gnss.set_altitude(static_cast<std::int64_t>(*currentGps.altitudeM * 1e6));
			}
			if (currentGps.satellites.has_value())
			{
				gnss.set_number_of_space_vehicles(*currentGps.satellites);
			}
			if (currentGps.hdop.has_value())
			{
				gnss.set_horizontal_dilution_of_precision(static_cast<std::int16_t>(*currentGps.hdop * 100.0));
			}
			gnss.set_gnss_method(mapFixQualityToGnssMethod(currentGps.fixQuality));
		}
		nmea2000->update();
	}



	void TcBridge::refreshFieldNames()
	{
		fieldIds.clear();
		currentFieldNames.clear();
		for (const auto &field : fieldTaskManager.list_fields())
		{
			fieldIds.push_back(field.id);
			currentFieldNames.push_back(QString::fromStdString(field.name));
		}
		emit fieldsChanged();
	}

	void TcBridge::refreshTaskNames()
	{
		taskIds.clear();
		currentTaskNames.clear();
		for (const auto &task : fieldTaskManager.list_tasks())
		{
			taskIds.push_back(task.id);
			currentTaskNames.push_back(QString::fromStdString(task.name));
		}
		emit tasksChanged();
	}

	void TcBridge::updateFieldSelection()
	{
		if ((currentSelectedField < 0) || (currentSelectedField >= static_cast<int>(fieldIds.size())))
		{
			return;
		}
		const auto field = fieldTaskManager.get_field(fieldIds[static_cast<std::size_t>(currentSelectedField)]);
		if (!field || field->exteriorRing.empty())
		{
			return;
		}
		std::size_t pointCount = field->exteriorRing.size();
		if ((pointCount > 1) && (field->exteriorRing.front() == field->exteriorRing.back()))
		{
			--pointCount;
		}
		double latitudeSum = 0.0;
		double longitudeSum = 0.0;
		for (std::size_t index = 0; index < pointCount; ++index)
		{
			latitudeSum += field->exteriorRing[index].first;
			longitudeSum += field->exteriorRing[index].second;
		}
		fieldOriginLatitude = latitudeSum / static_cast<double>(pointCount);
		fieldOriginLongitude = longitudeSum / static_cast<double>(pointCount);
		fieldOriginValid = true;
		double minimumX = 0.0;
		double maximumX = 0.0;
		double minimumNorth = 0.0;
		double maximumNorth = 0.0;
		for (std::size_t index = 0; index < pointCount; ++index)
		{
			const double x = (field->exteriorRing[index].second - fieldOriginLongitude) * DegreesToRadians *
			                 EarthRadiusM * std::cos(fieldOriginLatitude * DegreesToRadians);
			const double north = (field->exteriorRing[index].first - fieldOriginLatitude) * DegreesToRadians * EarthRadiusM;
			if (0 == index)
			{
				minimumX = maximumX = x;
				minimumNorth = maximumNorth = north;
			}
			else
			{
				minimumX = std::min(minimumX, x);
				maximumX = std::max(maximumX, x);
				minimumNorth = std::min(minimumNorth, north);
				maximumNorth = std::max(maximumNorth, north);
			}
		}
		currentFieldWidthM = maximumX - minimumX;
		currentFieldLengthM = maximumNorth - minimumNorth;
		currentActiveFieldName = QString::fromStdString(field->name);
		clearTrack();
		rebuildFieldBoundaryPoints();
	}

	void TcBridge::registerGpsCanCallbacks()
	{
		if (gpsCanCallbacksRegistered)
		{
			return;
		}
		for (const std::uint32_t pgn : { GpsPositionPgn, GpsPositionDeltaPgn, GpsPositionDeltaHighPrecisionPgn,
		                                 GpsPositionCovariancePgn, GpsPositionDeltaCovariancePgn })
		{
			isobus::CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(
			  pgn, processGpsCanMessage, this);
		}
		gpsCanCallbacksRegistered = true;
	}

	void TcBridge::unregisterGpsCanCallbacks()
	{
		if (!gpsCanCallbacksRegistered)
		{
			return;
		}
		for (const std::uint32_t pgn : { GpsPositionPgn, GpsPositionDeltaPgn, GpsPositionDeltaHighPrecisionPgn,
		                                 GpsPositionCovariancePgn, GpsPositionDeltaCovariancePgn })
		{
			isobus::CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(
			  pgn, processGpsCanMessage, this);
		}
		gpsCanCallbacksRegistered = false;
	}

	void TcBridge::processGpsCanMessage(const isobus::CANMessage &message, void *parentPointer)
	{
		auto *bridge = static_cast<TcBridge *>(parentPointer);
		if (nullptr == bridge)
		{
			return;
		}
		const auto &data = message.get_data();
		bridge->gpsProvider.feed_can_message(message.get_identifier().get_parameter_group_number(),
		                                     data.data(),
		                                     static_cast<std::uint8_t>(std::min<std::size_t>(data.size(), 255)));
	}

	void TcBridge::clearLog()
	{
		logs.clear();
	}
} // namespace agisotc
