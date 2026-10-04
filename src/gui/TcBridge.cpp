#include "TcBridge.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <map>

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
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

#include "PrescriptionJson.hpp"

namespace agisotc
{
	namespace
	{
		constexpr double EarthRadiusM = 6371000.0;
		constexpr double DegreesToRadians = 3.14159265358979323846 / 180.0;
		/// Booms whose LED bars lie closer than this fore and aft would hide each other in the views.
		constexpr double BoomLedOverlapM = 0.6;
		/// How far behind the one before an LED bar is drawn when its boom would hide it.
		constexpr double BoomLedSpacingM = 0.9;
		/// Share of a section's width its LED takes, so neighbouring LEDs stay apart.
		constexpr double BoomLedFill = 0.88;

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

		// The CAN stack's own log goes to the event log, from Info up; AGISOTC_STACK_LOG=debug also
		// writes all of it, Debug included, and the server's own lines to the debug output (stderr
		// with QT_FORCE_STDERR_LOGGING).
		stackLogVerbose = (0 == qEnvironmentVariable("AGISOTC_STACK_LOG").compare("debug", Qt::CaseInsensitive));
		isobus::CANStackLogger::set_can_stack_logger_sink(&stackLog);
		isobus::CANStackLogger::set_log_level(stackLogVerbose ? isobus::CANStackLogger::LoggingLevel::Debug
		                                                      : isobus::CANStackLogger::LoggingLevel::Info);
	}

	TcBridge::~TcBridge()
	{
		stopGps();
		stopServer();
		isobus::CANStackLogger::set_can_stack_logger_sink(nullptr);
	}

	void TcBridge::drainStackLog()
	{
		using Level = isobus::CANStackLogger::LoggingLevel;
		for (const auto &line : stackLog.take_lines())
		{
			const QString text = QString::fromStdString(line.text).trimmed();
			if (stackLogVerbose)
			{
				qInfo().noquote() << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "[stack]" << text;
			}
			if (line.level < Level::Info)
			{
				continue;
			}
			const QString severity = (line.level >= Level::Error) ? "ERROR: " : ((Level::Warning == line.level) ? "WARNING: " : "");
			logs.addLine(QString("[stack] %1%2").arg(severity, text));
		}
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

	QVariantMap TcBridge::implementLoading() const { return currentImplementLoading; }
	bool TcBridge::implementReady() const { return implementReadyFlag; }
	QString TcBridge::implementName() const { return currentImplementName; }
	QString TcBridge::implementGeometryStatus() const { return currentImplementGeometryStatus; }
	QVariantList TcBridge::implementElements() const { return currentImplementElements; }
	QVariantList TcBridge::booms() const { return currentBooms; }
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

	void TcBridge::drainBusFrames()
	{
		if (!canBus.is_running())
		{
			return;
		}
		const auto ourControl = canBus.internal_control_function();
		const int ourAddress = (nullptr != ourControl) ? static_cast<int>(ourControl->get_address()) : -1;
		for (const auto &frame : canBus.take_sniffed_frames())
		{
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
			if ((0xEE00 == pgn) && (frame.length >= 8) && !frame.outgoing && (source >= 0) && (source <= 253))
			{
				// An address claim: who is on the bus, for the clients panel.
				std::uint64_t rawName = 0;
				for (std::uint8_t i = 0; i < 8; ++i)
				{
					rawName |= static_cast<std::uint64_t>(frame.data[i]) << (8 * i);
				}
				const isobus::NAME claimed(rawName);
				BusPeerInfo info;
				info.functionCode = claimed.get_function_code();
				info.functionInstance = claimed.get_function_instance();
				info.manufacturerCode = claimed.get_manufacturer_code();
				info.lastSeenMs = steady_clock_ms();
				busPeersByAddress[static_cast<std::uint8_t>(source)] = info;
				refreshBusPeers();
				connectionProgress.on_address_claim(static_cast<std::uint8_t>(source), rawName, info.lastSeenMs);
			}
			// Implements connecting: their working set master message and all they send this TC,
			// the DDOP upload's transport frames included.
			if (!frame.outgoing && (source >= 0) && (source <= 253))
			{
				if (0xFE0D == pgn)
				{
					connectionProgress.on_working_set_master(static_cast<std::uint8_t>(source), frame.timestampMs);
				}
				else if ((destination == ourAddress) && (frame.identifier > 0x7FF))
				{
					connectionProgress.on_frame_to_tc(static_cast<std::uint8_t>(source), pgn, frame.data, frame.length, frame.timestampMs);
				}
			}
			if ((0xCB00 == pgn) && (8 == frame.length) && (frame.identifier > 0x7FF))
			{
				if (!frame.outgoing && (destination == ourAddress) && (0x10 == frame.data[0]))
				{
					// A client's version message: say when it has more than this TC offers, since
					// a client then limits the booms, sections or channels it lets the TC control.
					const int clientBooms = frame.data[5];
					const int clientSections = frame.data[6];
					const int clientChannels = frame.data[7];
					QString line = QString("[tc] Client %1 reports TC version %2: %3 booms, %4 sections, %5 channels (this TC: %6, %7, %8).")
					                 .arg(source).arg(frame.data[1]).arg(clientBooms).arg(clientSections).arg(clientChannels)
					                 .arg(supportedBooms).arg(supportedSections).arg(supportedChannels);
					if ((clientBooms > supportedBooms) || (clientSections > supportedSections) || (clientChannels > supportedChannels))
					{
						line += " The client may hold back what exceeds this; restart the server with larger numbers to offer it all.";
					}
					logs.addLine(line);
				}
				if ((source == ourAddress) || (destination == ourAddress))
				{
					const std::uint8_t command = frame.data[0] & 0x0F;
					const std::uint16_t element = static_cast<std::uint16_t>((frame.data[0] >> 4) | (frame.data[1] << 4));
					const std::uint16_t ddi = static_cast<std::uint16_t>(frame.data[2] | (frame.data[3] << 8));
					const std::int32_t value = static_cast<std::int32_t>(frame.data[4] | (frame.data[5] << 8) | (frame.data[6] << 16) | (frame.data[7] << 24));
					ingestSniffedProcessData(source, destination, frame.outgoing, command, ddi, element, value);
				}
			}
		}
		// Drop peers silent for over a minute.
		const std::uint64_t nowMs = steady_clock_ms();
		bool peersChanged = false;
		for (auto it = busPeersByAddress.begin(); it != busPeersByAddress.end();)
		{
			if ((nowMs - it->second.lastSeenMs) > 60000)
			{
				it = busPeersByAddress.erase(it);
				peersChanged = true;
			}
			else
			{
				++it;
			}
		}
		if (peersChanged)
		{
			refreshBusPeers();
		}
	}

	void TcBridge::refreshBusPeers()
	{
		QVariantList peers;
		for (const auto &entry : busPeersByAddress)
		{
			QVariantMap row;
			row["address"] = static_cast<int>(entry.first);
			row["functionCode"] = entry.second.functionCode;
			row["functionInstance"] = entry.second.functionInstance;
			row["manufacturerCode"] = entry.second.manufacturerCode;
			row["connected"] = (connectedAddresses.find(entry.first) != connectedAddresses.end());
			peers.push_back(row);
		}
		currentBusPeers = peers;
		emit busPeersChanged();
	}

	QVariantList TcBridge::busPeers() const
	{
		return currentBusPeers;
	}

	void TcBridge::ingestSniffedProcessData(int source, int destination, bool outgoing,
	                                        std::uint8_t command, std::uint16_t ddi,
	                                        std::uint16_t element, std::int32_t value)
	{
		// Single decode path for process data seen on the bus: feeds the DDI
		// traffic view, the Raw tab, the implement model, and section states,
		// whether or not the sender completed the TC connection procedure.
		const QString direction = outgoing ? "TC -> client" : "client -> TC";
		const int peerAddress = outgoing ? destination : source;

		QString commandName;
		bool valueBearing = false;
		switch (command)
		{
			case 0x02: commandName = "Request"; break;
			case 0x03: commandName = "Value"; valueBearing = true; break;
			case 0x04: commandName = "MeasTime"; break;
			case 0x05: commandName = "MeasDist"; break;
			case 0x06: commandName = "MeasMin"; break;
			case 0x07: commandName = "MeasMax"; break;
			case 0x08: commandName = "MeasChg"; break;
			case 0x0A: commandName = "Set+Ack"; valueBearing = true; break;
			case 0x0D: commandName = "Ack"; break;
			case 0x0E: commandName = "Status"; break;
			case 0x0F: commandName = "ClientTask"; break;
			case 0x09: commandName = "PeerCtl"; break;
			default: commandName = QString("Cmd%1").arg(command); break;
		}
		if ((0x00 == command) || (0x01 == command))
		{
			return; // Technical / device-descriptor transfers are bus-tab material only.
		}

		QString detail = "sniffed";
		for (const auto &state : implementDdiStates)
		{
			if ((state.ddi == ddi) && (state.element == element))
			{
				detail = state.name;
				break;
			}
		}
		if (liveDdiTrafficWatchEnabled)
		{
			appendDdiTraffic(direction, commandName, peerAddress, ddi, element, value, detail);
		}
		if (!valueBearing)
		{
			return;
		}
		values.upsertValue(peerAddress, ddi, element, value, timestamp_now());
		if (peerAddress != currentSelectedClient)
		{
			return;
		}
		updateImplementValue(ddi, element, value);
		if ((0 != currentSectionDdi) && (ddi == currentSectionDdi) &&
		    (element >= 1) && (element <= currentSectionCount))
		{
			const bool on = (0 != value);
			if (currentSectionStates.at(element - 1).toBool() != on)
			{
				currentSectionStates[element - 1] = QVariant(on);
				emit sectionStatesChanged();
			}
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
		else if ("pcan_virtual" == settings.driver)
		{
			logs.addLine(QString("[bus] PCAN Virtual network '%1' stays registered after exit. Use the same "
			                     "network name in the VT and implement simulator; they can start in any order.")
			               .arg(QString::fromStdString(settings.channel)));
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

		supportedBooms = booms;
		supportedSections = sections;
		supportedChannels = channels;
		server = std::make_shared<GuiTaskControllerServer>(
		  canBus.internal_control_function(),
		  static_cast<std::uint8_t>(booms),
		  static_cast<std::uint8_t>(sections),
		  static_cast<std::uint8_t>(channels),
		  options);
		server->get_language_command_interface().set_language_code("en");
		server->get_language_command_interface().set_country_code("US");
		server->initialize();
		server->start_client_recovery();
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
		// Hand the client back to manual control rather than leaving it following a TC that is gone.
		if (sectionController.engaged())
		{
			sendTcCommands(sectionController.set_engaged(false, steady_clock_ms()));
		}
		if (rateController.any_engaged())
		{
			sendTcCommands(rateController.set_engaged({}, steady_clock_ms()));
		}
		pumpRunning = false;
		if (pumpThread.joinable())
		{
			pumpThread.join();
		}
		if (nullptr != server)
		{
			server->stop_client_recovery();
			server->terminate();
			server.reset();
		}
		speedMessages.reset();
		nmea2000.reset();
		unregisterGpsCanCallbacks();
		canBus.stop();
		for (auto &state : implementDdiStates) state.reportingConfigured = false;
		lastDdiSyncMs = 0;
		sectionController.reset({});
		planClient = -1;
		pendingSetupAtMs = 0;
		currentSectionControlStatus = "No client";
		emit sectionControlChanged();
		setupRatePlan(nullptr, false);
		running = false;
		emit runningChanged();

		// No server, no connections: the clients, their implement and what was connecting go.
		connectionProgress.reset();
		clientLinks.clear();
		connectedAddresses.clear();
		busPeersByAddress.clear();
		refreshBusPeers();
		clients.setClients({});
		selectedClientOnline = false;
		manualPool.clear();
		manualPoolClient = -1;
		if (-1 != currentSelectedClient)
		{
			currentSelectedClient = -1;
			emit selectedClientChanged();
		}
		refreshDdop();
		serviceImplementLoading(steady_clock_ms());
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
				server->recover_unknown_clients();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}

	void TcBridge::poll()
	{
		drainStackLog();
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
			if (stackLogVerbose)
			{
				qInfo().noquote() << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "[server]" << QString::fromStdString(line);
			}
			logs.addLine(QString("[%1] %2").arg(timestamp_now(), QString::fromStdString(line)));
		}
		// Process-data values, section states, and DDI traffic rows all come
		// from the sniffer decode now (ingestSniffedProcessData), which also
		// covers senders the protocol stack drops.
		if (events.rosterChanged)
		{
			refreshClients();
		}
		else
		{
			// Refresh lightweight fields (last-seen, status) while keeping selection stable.
			refreshClients();
		}
		bool planNeeded = (planClient != currentSelectedClient);
		if (!events.poolsChanged.empty())
		{
			for (const auto address : events.poolsChanged)
			{
				if (static_cast<int>(address) == currentSelectedClient)
				{
					manualPool.clear();
					manualPoolClient = -1;
					refreshDdop();
					planNeeded = true; // the client (re)activated its pool
				}
			}
		}
		if (planNeeded)
		{
			setupClientPlan();
		}
		if (events.identifyRequested)
		{
			emit identifyBanner(events.identifyNumber);
		}
		serviceDdiSync();

		const auto nowMs = steady_clock_ms();
		if ((0 != pendingSetupAtMs) && (nowMs >= pendingSetupAtMs))
		{
			pendingSetupAtMs = 0;
			sendSetupCommands();
			// A client may take the Prescription Control State only once it sees the task active.
			sendTcCommands(rateController.reassert(nowMs));
		}
		serviceSectionControl(nowMs);
		serviceRateControl(nowMs);
		publishImplementModelIfDue(nowMs);
		serviceGeometryRequests(nowMs);
		serviceImplementLoading(nowMs);
	}

	bool TcBridge::isShownElement(const ImplementElementState &element) const
	{
		using Type = isobus::task_controller_object::DeviceElementObject::Type;
		const auto type = static_cast<Type>(element.type);
		if ((Type::Connector == type) || (Type::Section == type)) return true;
		if (Type::Device == type) return false;
		const bool anySections = std::any_of(implementElementStates.cbegin(), implementElementStates.cend(),
		                                     [](const ImplementElementState &other) { return static_cast<Type>(other.type) == Type::Section; });
		if (!anySections) return true; // nothing else to draw the implement with
		// A boom (or sub-boom) is an element with sections below it.
		for (const auto &other : implementElementStates)
		{
			if (static_cast<Type>(other.type) != Type::Section) continue;
			std::uint16_t parent = other.parentObjectId;
			for (int depth = 0; depth < 16; ++depth)
			{
				if (parent == element.objectId) return true;
				const auto next = std::find_if(implementElementStates.cbegin(), implementElementStates.cend(),
				                               [parent](const ImplementElementState &candidate) { return candidate.objectId == parent; });
				if ((next == implementElementStates.cend()) || (next->parentObjectId == parent)) break;
				parent = next->parentObjectId;
			}
		}
		return false;
	}

	void TcBridge::publishImplementModelIfDue(std::uint64_t nowMs)
	{
		// Commanded section states change without a value from the client: the LED bars follow them too.
		if (!implementModelDirty && (sectionController.plan().section_count() > 0) && (appliedSectionStates() != publishedLedStates))
		{
			implementModelDirty = true;
		}
		if (implementModelDirty && ((nowMs - lastImplementPublishMs) >= IMPLEMENT_PUBLISH_MS))
		{
			publishImplementModel();
		}
	}

	void TcBridge::refreshClients()
	{
		QList<ClientRow> rows;
		connectedAddresses.clear();
		const std::uint64_t nowMs = steady_clock_ms();
		for (const auto &snapshot : server->clients_snapshot())
		{
			if (snapshot.ddopActive && !snapshot.timedOut && (snapshot.address <= 253))
			{
				connectedAddresses.insert(static_cast<std::uint8_t>(snapshot.address));
			}
			// An activated pool lets the implement be built; a time-out ends its connection.
			auto &link = clientLinks[snapshot.address];
			const bool active = snapshot.ddopActive && !snapshot.timedOut;
			if ((snapshot.address <= 253) && active && !link.active)
			{
				connectionProgress.on_pool_activated(static_cast<std::uint8_t>(snapshot.address), nowMs);
			}
			if ((snapshot.address <= 253) && snapshot.timedOut && !link.timedOut)
			{
				connectionProgress.on_client_lost(static_cast<std::uint8_t>(snapshot.address));
			}
			link.active = active;
			link.timedOut = snapshot.timedOut;
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

		// Without a connection there is no implement: a client that timed out or deactivated its
		// pool takes it out of the views. It comes back when the client activates its pool again.
		const bool online = (-1 != currentSelectedClient) &&
		  (connectedAddresses.find(static_cast<std::uint8_t>(currentSelectedClient)) != connectedAddresses.end());
		const bool wentOffline = selectedClientOnline && !online;
		selectedClientOnline = online;
		if (wentOffline)
		{
			refreshDdop();
		}
		refreshBusPeers();
	}

	void TcBridge::refreshDdop()
	{
		QList<DdopRow> rows;
		std::vector<std::uint8_t> binary;
		clearImplementModel();

		const bool manual = (manualPoolClient == currentSelectedClient) && !manualPool.empty();
		if (manual)
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
		else if (!manual && !binary.empty() && !isClientOnline(currentSelectedClient))
		{
			// The pool stays stored, so the client need not upload it again, but a client that is not
			// connected shows no objects.
			rows.push_back({ 0, QString("Client %1 is not connected.").arg(currentSelectedClient) });
			rows.push_back({ 0, "Its device descriptor shows here again when it connects." });
		}
		else if (binary.empty())
		{
			rows.push_back({ 0, "No DDOP received from this client yet." });
			rows.push_back({ 0, "It appears here automatically once the client uploads its pool," });
			rows.push_back({ 0, "or load a pool file manually with the upload button above." });
		}
		else
		{
			isobus::DeviceDescriptorObjectPool pool;
			const auto clientVersion = (nullptr != server) ? server->client_version(static_cast<std::uint8_t>(currentSelectedClient)) : 0;
			if (0 == parse_client_pool(binary, clientVersion, pool))
			{
				rows.push_back({ 0, QString("Stored %1 bytes, but parsing failed.").arg(binary.size()) });
			}
			else
			{
				// The implement is built only for a connected client; the stored pool of one that is
				// gone stays listed here.
				if (isClientOnline(currentSelectedClient))
				{
					buildImplementModel(pool);
					// Pull live values immediately so the Raw tab fills without
					// waiting for the trickle sync; quiet implements only answer.
					requestImplementDdis();
					lastGeometryRequestMs = steady_clock_ms();
				}
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
		currentBooms.clear();
		boomLedRows.clear();
		implementElementRows.clear();
		publishedLedStates.clear();
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
			    (ddi == static_cast<std::uint16_t>(DDI::MaximumWorkingWidth)) ||
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
			for (const auto childId : child_object_ids(*element))
			{
				auto child = pool.get_object_by_id(childId);
				if (auto *property = dynamic_cast<DevicePropertyObject *>(child.get()))
				{
					const int kind = geometryKindForDdi(property->get_ddi());
					if (0 == kind) continue;
					const double metres = geometry_value_metres(pool, property->get_value(), property->get_device_value_presentation_object_id());
					switch (kind)
					{
						case 1: elementState.localX = metres; elementState.hasOffsetX = true; break;
						case 2: elementState.localY = metres; elementState.hasOffsetY = true; break;
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
		// ISO 11783-10 gives element offsets from the device reference point, not from the parent
		// element; an element without an offset of its own sits where the element above it is.
		std::array<double, 2> connectorOffset = { 0.0, 0.0 };
		double connectorZ = 0.0;
		for (const auto &element : implementElementStates)
		{
			if (element.type == static_cast<int>(isobus::task_controller_object::DeviceElementObject::Type::Connector))
			{
				connectorOffset = elementOffset(element);
				connectorZ = element.localZ;
				break;
			}
		}

		for (const auto &element : implementElementStates)
		{
			if (!isShownElement(element)) continue;
			const auto offset = elementOffset(element);
			QVariantMap row;
			row["objectId"] = element.objectId;
			row["element"] = element.element;
			row["name"] = element.name;
			row["type"] = element.type;
			// ISO: X forward, Y to the right, Z vertical. Scene (front is -Z): X right, Z rearward.
			row["x"] = offset[1] - connectorOffset[1];
			row["y"] = element.localZ - connectorZ;
			row["z"] = -(offset[0] - connectorOffset[0]);
			row["width"] = element.width;
			row["length"] = element.length;
			row["height"] = element.height;
			row["active"] = element.active;
			row["hasGeometry"] = element.hasGeometry;
			elements.push_back(row);
		}
		currentImplementElements = elements;
		implementElementRows.setRows(elements);
		publishBoomLeds(connectorOffset);
		implementModelDirty = false;
		lastImplementPublishMs = steady_clock_ms();

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

	void TcBridge::publishBoomLeds(const std::array<double, 2> &connectorOffset)
	{
		using Type = isobus::task_controller_object::DeviceElementObject::Type;
		const auto byElement = [this](std::uint16_t number) -> const ImplementElementState * {
			const auto found = std::find_if(implementElementStates.cbegin(), implementElementStates.cend(),
			                                [number](const ImplementElementState &element) { return element.element == number; });
			return (found == implementElementStates.cend()) ? nullptr : &*found;
		};

		// The booms with their sections: as the TC plan has them (condensed work state order, the
		// states as the client reports them or else as commanded), or, before there is a plan, the
		// sections grouped under the element they hang from, with the states they report.
		struct BoomLeds
		{
			const ImplementElementState *boom = nullptr;
			std::vector<const ImplementElementState *> sections;
			std::vector<bool> on;
		};
		std::vector<BoomLeds> booms;
		const auto &plan = sectionController.plan();
		publishedLedStates.clear();
		if (plan.section_count() > 0)
		{
			publishedLedStates = appliedSectionStates();
			std::size_t flat = 0;
			for (const auto &boomPlan : plan.booms)
			{
				BoomLeds boom;
				boom.boom = byElement(boomPlan.element);
				for (const auto number : boomPlan.sections)
				{
					const bool on = (flat < publishedLedStates.size()) && publishedLedStates[flat];
					++flat;
					const auto *section = byElement(number);
					if (nullptr == section) continue;
					boom.sections.push_back(section);
					boom.on.push_back(on);
				}
				if (!boom.sections.empty()) booms.push_back(boom);
			}
		}
		else
		{
			std::map<std::uint16_t, BoomLeds> byParent;
			for (const auto &element : implementElementStates)
			{
				if (element.type == static_cast<int>(Type::Section)) byParent[element.parentObjectId].sections.push_back(&element);
			}
			for (auto &[parent, boom] : byParent)
			{
				for (const auto &candidate : implementElementStates)
				{
					if (candidate.objectId == parent) boom.boom = &candidate;
				}
				std::sort(boom.sections.begin(), boom.sections.end(), [](const auto *left, const auto *right) { return left->element < right->element; });
				for (const auto *section : boom.sections) boom.on.push_back(section->active);
				booms.push_back(boom);
			}
		}

		// One LED per section, as wide as the section and where it is across the implement, on a
		// bar as wide as the boom; the 3D view draws the bars where the booms trail the hitch.
		QVariantList boomRows;
		QVariantList ledRows;
		std::vector<double> barDepths;
		for (std::size_t index = 0; index < booms.size(); ++index)
		{
			const auto &boom = booms[index];
			struct Led
			{
				double x = 0.0;
				double z = 0.0;
				double width = 0.0;
			};
			std::vector<Led> leds;
			double left = std::numeric_limits<double>::max();
			double right = std::numeric_limits<double>::lowest();
			double depthSum = 0.0;
			for (const auto *section : boom.sections)
			{
				const auto offset = elementOffset(*section);
				Led led;
				led.x = offset[1] - connectorOffset[1];
				led.z = -(offset[0] - connectorOffset[0]);
				led.width = std::max(section->width, 0.05);
				left = std::min(left, led.x - (led.width / 2.0));
				right = std::max(right, led.x + (led.width / 2.0));
				depthSum += led.z;
				leds.push_back(led);
			}
			double barZ = depthSum / static_cast<double>(leds.size());
			for (bool moved = true; moved;)
			{
				moved = false;
				for (const double other : barDepths)
				{
					if (std::abs(other - barZ) < BoomLedOverlapM)
					{
						barZ = other + BoomLedSpacingM;
						moved = true;
					}
				}
			}
			barDepths.push_back(barZ);

			const QString name = ((nullptr != boom.boom) && !boom.boom->name.isEmpty()) ? boom.boom->name : QString("Boom %1").arg(index + 1);
			QVariantMap rail;
			rail["kind"] = QString("rail");
			rail["boom"] = static_cast<int>(index);
			rail["number"] = 0;
			rail["x"] = (left + right) / 2.0;
			rail["z"] = barZ;
			rail["width"] = right - left;
			rail["on"] = false;
			ledRows.push_back(rail);

			QVariantList sectionRows;
			int onCount = 0;
			for (std::size_t s = 0; s < leds.size(); ++s)
			{
				const bool on = boom.on[s];
				onCount += on ? 1 : 0;
				QVariantMap led;
				led["kind"] = QString("led");
				led["boom"] = static_cast<int>(index);
				led["number"] = static_cast<int>(s + 1);
				led["x"] = leds[s].x;
				led["z"] = barZ;
				led["width"] = leds[s].width * BoomLedFill;
				led["on"] = on;
				ledRows.push_back(led);

				QVariantMap section;
				section["number"] = static_cast<int>(s + 1);
				section["element"] = boom.sections[s]->element;
				section["name"] = boom.sections[s]->name;
				section["left"] = leds[s].x - (leds[s].width / 2.0);
				section["width"] = leds[s].width;
				section["on"] = on;
				sectionRows.push_back(section);
			}
			QVariantMap row;
			row["index"] = static_cast<int>(index);
			row["element"] = (nullptr != boom.boom) ? boom.boom->element : 0;
			row["name"] = name;
			row["count"] = static_cast<int>(leds.size());
			row["onCount"] = onCount;
			row["left"] = left;
			row["right"] = right;
			row["widthM"] = right - left;
			row["z"] = barZ;
			row["sections"] = sectionRows;
			boomRows.push_back(row);
		}
		currentBooms = boomRows;
		boomLedRows.setRows(ledRows);
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
						case 1: geometry.localX = metres; geometry.hasOffsetX = true; break;
						case 2: geometry.localY = metres; geometry.hasOffsetY = true; break;
						case 3: geometry.localZ = metres; break;
						case 4: geometry.width = std::abs(metres); break;
						case 5: geometry.length = std::abs(metres); break;
						case 6: geometry.height = std::abs(metres); break;
					}
					geometry.hasGeometry = true;
				}
				currentImplementGeometryStatus = QString("Geometry from the client's process data, %1 elements").arg(implementElementStates.size());
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
		if (is_actual_condensed_work_state(ddi))
		{
			// A condensed work state covers the sections of the boom that reports it, so with
			// several booms each one's value applies to its own sections only.
			std::vector<std::uint16_t> boomSections;
			for (const auto &boom : sectionController.plan().booms)
			{
				if (boom.element == element) boomSections = boom.sections;
			}
			if (boomSections.empty())
			{
				// No plan for this element: fall back to the device's sections in element order.
				for (const auto &geometry : implementElementStates)
				{
					if (geometry.type == static_cast<int>(isobus::task_controller_object::DeviceElementObject::Type::Section)) boomSections.push_back(geometry.element);
				}
				std::sort(boomSections.begin(), boomSections.end());
			}
			const auto firstCondensed = static_cast<std::uint16_t>(DDI::ActualCondensedWorkState1_16);
			const std::size_t base = static_cast<std::size_t>(ddi - firstCondensed) * 16;
			const auto states = decode_condensed_work_state(static_cast<std::uint32_t>(value), 16);
			for (std::size_t i = 0; (i < states.size()) && ((base + i) < boomSections.size()); ++i)
			{
				for (auto &geometry : implementElementStates)
				{
					if (geometry.element == boomSections[base + i]) geometry.active = states[i];
				}
			}
			changed = true;
		}
		// Values arrive by the dozen per second: the views are refreshed from poll() instead.
		if (changed) implementModelDirty = true;
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
			// Request Default Process Data is a command, not a value: the plan's set-up sends it.
			if (state.ddi == static_cast<std::uint16_t>(isobus::DataDescriptionIndex::RequestDefaultProcessData)) continue;
			if (server->send_request_value(client, state.ddi, state.element))
			{
				++sentCount;
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
			if (state.ddi == static_cast<std::uint16_t>(isobus::DataDescriptionIndex::RequestDefaultProcessData))
			{
				++nextDdiSyncIndex; // a command, not a value to poll
				continue;
			}
			if (!state.reportingConfigured && (state.triggers & static_cast<std::uint8_t>(Trigger::TimeInterval)))
			{
				state.reportingConfigured = server->send_time_interval_measurement_command(
				  client, state.ddi, state.element, static_cast<std::uint32_t>(currentDdiSyncIntervalMs));
			}
			server->send_request_value(client, state.ddi, state.element);
			++nextDdiSyncIndex;
		}
		lastDdiSyncMs = now;
	}

	bool TcBridge::isClientOnline(int address)
	{
		if (!running || (nullptr == server) || (address < 0))
		{
			return false;
		}
		for (const auto &snapshot : server->clients_snapshot())
		{
			if (static_cast<int>(snapshot.address) == address)
			{
				return snapshot.ddopActive && !snapshot.timedOut;
			}
		}
		return false;
	}

	void TcBridge::serviceGeometryRequests(std::uint64_t nowMs)
	{
		if (!selectedClientOnline || connectionProgress.is_ready(static_cast<std::uint8_t>(currentSelectedClient)) ||
		    ((nowMs - lastGeometryRequestMs) < GEOMETRY_REQUEST_MS))
		{
			return;
		}
		auto client = server->find_client(static_cast<std::uint8_t>(currentSelectedClient));
		if (nullptr == client)
		{
			return;
		}
		constexpr int MAX_REQUESTS = 16;
		int sent = 0;
		for (const auto &state : implementDdiStates)
		{
			if ((0 != state.geometryKind) && !state.hasValue && (sent < MAX_REQUESTS))
			{
				server->send_request_value(client, state.ddi, state.element);
				++sent;
			}
		}
		lastGeometryRequestMs = nowMs;
	}

	void TcBridge::serviceImplementLoading(std::uint64_t nowMs)
	{
		// The geometry the selected client's implement still waits for.
		if (selectedClientOnline && !implementElementStates.empty())
		{
			int received = 0;
			int total = 0;
			for (const auto &state : implementDdiStates)
			{
				if (0 == state.geometryKind) continue;
				++total;
				received += state.hasValue ? 1 : 0;
			}
			connectionProgress.on_geometry(static_cast<std::uint8_t>(currentSelectedClient), received, total, nowMs);
		}

		const auto state = connectionProgress.current(nowMs);
		QVariantMap loading;
		loading["active"] = (ConnectionProgress::Step::None != state.step);
		loading["step"] = static_cast<int>(state.step);
		loading["progress"] = state.progress;
		loading["address"] = state.address;
		loading["transferBytes"] = static_cast<qlonglong>(state.transferBytes);
		loading["receivedBytes"] = static_cast<qlonglong>(state.receivedBytes);
		loading["uploadSkipped"] = state.uploadSkipped;
		loading["geometryReceived"] = state.geometryReceived;
		loading["geometryTotal"] = state.geometryTotal;
		loading["elapsedMs"] = static_cast<qlonglong>(state.elapsedMs);
		loading["name"] = ((state.address == currentSelectedClient) && (state.step >= ConnectionProgress::Step::Building)) ? currentImplementName : QString();
		if (loading != currentImplementLoading)
		{
			currentImplementLoading = loading;
			emit implementLoadingChanged();
		}

		ConnectionProgress::Timings timings;
		while (connectionProgress.take_completed(timings))
		{
			const auto seconds = [](std::uint64_t ms) { return QString::number(static_cast<double>(ms) / 1000.0, 'f', 1) + " s"; };
			QStringList steps;
			if (timings.startUpMs > 0) steps << "start-up " + seconds(timings.startUpMs);
			steps << "connection " + seconds(timings.connectMs);
			if (timings.uploadedBytes > 0)
			{
				steps << QString("DDOP upload %1 (%2 kB)").arg(seconds(timings.uploadMs)).arg(static_cast<double>(timings.uploadedBytes) / 1024.0, 0, 'f', 1);
			}
			else
			{
				steps << "stored DDOP, no upload";
			}
			steps << "geometry " + seconds(timings.buildMs);
			logs.addLine(QString("[connect] Implement %1 ready in %2: %3.").arg(timings.address).arg(seconds(timings.totalMs), steps.join(", ")));
		}

		const bool ready = selectedClientOnline && connectionProgress.is_ready(static_cast<std::uint8_t>(currentSelectedClient));
		if (ready != implementReadyFlag)
		{
			implementReadyFlag = ready;
			emit implementReadyChanged();
		}
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
		const auto clientVersion = (nullptr != server) ? server->client_version(static_cast<std::uint8_t>(currentSelectedClient)) : 0;
		if (0 == parse_client_pool(binary, clientVersion, pool))
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
		refreshPrescription(true);
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
		// Many implements only report process data while a task is active, and may have dropped
		// their measurements when the last task stopped. They see the task only in the next TC
		// status message, so the set-up goes out now and once more after that message.
		requestImplementDdis();
		sendSetupCommands();
		pendingSetupAtMs = steady_clock_ms() + 2500;
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
			const auto prescription = taskPrescriptions.find(task.id);
			if ((prescription != taskPrescriptions.end()) && !prescription->second.empty())
			{
				object["prescription"] = prescription_to_json(prescription->second);
			}
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
			const std::string taskId = task.name.empty() ? std::string() : fieldTaskManager.create_task(task);
			if (!taskId.empty())
			{
				++loaded;
				if (object.contains("prescription"))
				{
					taskPrescriptions[taskId] = prescription_from_json(object.value("prescription").toObject());
				}
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
		refreshPrescription(true);
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
		coverage.clear();
		coveragePatchList.clear();
		changedPatches.clear();
		openPatchBySection.assign(openPatchBySection.size(), -1);
		sectionCentresValid = false;
		publishCoverage(true);
	}

	void TcBridge::rebuildFieldBoundaryPoints()
	{
		currentFieldBoundaryPoints.clear();
		boundaryLocal.clear();
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
			if (!boundaryRecordingFlag)
			{
				boundaryLocal.push_back({ point["x"].toDouble(), point["z"].toDouble() });
			}
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
		const auto nowMs = steady_clock_ms();
		const double distance = coveragePositionValid ? std::hypot(currentImplementX - lastCoverageX, currentImplementZ - lastCoverageZ) : 0.0;
		lastCoverageX = currentImplementX;
		lastCoverageZ = currentImplementZ;
		coveragePositionValid = true;

		// Coverage is what the sections applied: each section that is on paints the ground its
		// centre swept since the last step, over its own width.
		const auto poses = sectionPoses();
		const auto applied = appliedSectionStates();
		if (!sectionCentresValid || (lastSectionCentres.size() != poses.size()))
		{
			lastSectionCentres.clear();
			for (const auto &pose : poses) lastSectionCentres.push_back(pose.centre);
			openPatchBySection.assign(poses.size(), -1);
			sectionVelocities.assign(poses.size(), GroundPoint{});
			sectionCentresValid = true;
			return;
		}
		const bool jumped = distance > 10.0; // a position jump is not driven ground
		bool anyOn = false;
		for (std::size_t i = 0; i < poses.size(); ++i)
		{
			// How the section itself moved: in a turn the outer sections go faster than the inner
			// ones, and neither goes where the implement points. Smoothed over a few steps.
			if (jumped)
			{
				sectionVelocities[i] = {};
			}
			else if (elapsedSeconds > 0.0)
			{
				const GroundPoint measured = { (poses[i].centre.x - lastSectionCentres[i].x) / elapsedSeconds,
					                           (poses[i].centre.z - lastSectionCentres[i].z) / elapsedSeconds };
				sectionVelocities[i] = { (sectionVelocities[i].x + measured.x) / 2.0, (sectionVelocities[i].z + measured.z) / 2.0 };
			}
			const bool on = (i < applied.size()) && applied[i];
			if (on && !jumped)
			{
				coverage.cover_swath(lastSectionCentres[i], poses[i].centre, poses[i].widthM, nowMs);
				extendCoveragePatch(i, lastSectionCentres[i], poses[i].centre, poses[i].widthM);
				anyOn = true;
			}
			else
			{
				openPatchBySection[i] = -1;
			}
			lastSectionCentres[i] = poses[i].centre;
		}
		if (anyOn)
		{
			currentWorkedDistanceM += distance;
			currentWorkedTimeSeconds += elapsedSeconds;
		}
		currentWorkedAreaHa = coverage.covered_area_m2() / 10000.0;
		publishCoverage(false);
	}

	// --- TC controller ---------------------------------------------------------------------

	void TcBridge::setupClientPlan()
	{
		const auto nowMs = steady_clock_ms();
		if (sectionController.engaged())
		{
			sendTcCommands(sectionController.set_engaged(false, nowMs)); // previous client back to manual
		}
		if (rateController.any_engaged())
		{
			sendTcCommands(rateController.set_engaged({}, nowMs));
		}
		planClient = currentSelectedClient;
		ClientPlan plan;
		isobus::DeviceDescriptorObjectPool pool;
		bool parsed = false;
		if ((nullptr != server) && (planClient >= 0))
		{
			const auto address = static_cast<std::uint8_t>(planClient);
			if (0 != parse_client_pool(server->stored_pool(address), server->client_version(address), pool))
			{
				plan = build_client_plan(pool);
				parsed = true;
			}
		}
		sectionController.reset(plan);
		sectionCentresValid = false;

		if (plan.supports_section_control())
		{
			QStringList booms;
			for (const auto &boom : plan.booms)
			{
				booms << QString("element %1 with %2 sections").arg(boom.element).arg(boom.sections.size());
			}
			currentSectionControlStatus = QString("Ready: %1 boom(s), %2 sections").arg(plan.booms.size()).arg(plan.section_count());
			logs.addLine(QString("[tc-sc] Client %1 accepts section control: %2%3.")
			               .arg(planClient)
			               .arg(booms.join(", "))
			               .arg(plan.sectionControlStateElement ? QString(", auto/manual on element %1").arg(*plan.sectionControlStateElement) : QString()));
		}
		else
		{
			currentSectionControlStatus = (planClient < 0) ? QString("No client") : QString("The client's DDOP offers no section setpoints");
		}
		emit sectionControlChanged();
		setupRatePlan(parsed ? &pool : nullptr, false);
		sendSetupCommands();
	}

	void TcBridge::sendSetupCommands()
	{
		if ((planClient < 0) || (connectedAddresses.find(static_cast<std::uint8_t>(planClient)) == connectedAddresses.end())) return;
		auto commands = sectionController.plan().setupCommands;
		const auto &rateSetup = rateController.plan().setupCommands;
		commands.insert(commands.end(), rateSetup.cbegin(), rateSetup.cend());
		if (commands.empty()) return;
		sendTcCommands(commands);
		const auto triggers = std::count_if(commands.cbegin(), commands.cend(), [](const TcCommand &command) {
			return TcCommand::Kind::ChangeThreshold == command.kind;
		});
		const bool defaultData = std::any_of(commands.cbegin(), commands.cend(), [](const TcCommand &command) {
			return command.ddi == static_cast<std::uint16_t>(isobus::DataDescriptionIndex::RequestDefaultProcessData);
		});
		const auto intervals = std::count_if(commands.cbegin(), commands.cend(), [](const TcCommand &command) {
			return TcCommand::Kind::TimeInterval == command.kind;
		});
		logs.addLine(QString("[tc] Set up client %1: %2%3 on-change report(s)%4.")
		               .arg(planClient)
		               .arg(defaultData ? "requested its default process data (TC-BAS), " : "")
		               .arg(triggers)
		               .arg((intervals > 0) ? QString(", %1 actual rate(s) every %2 ms").arg(intervals).arg(ACTUAL_RATE_INTERVAL_MS) : QString()));
	}

	void TcBridge::sendTcCommands(const std::vector<TcCommand> &commands)
	{
		if ((nullptr == server) || (planClient < 0) || commands.empty()) return;
		auto client = server->find_client(static_cast<std::uint8_t>(planClient));
		if (nullptr == client) return;
		for (const auto &command : commands)
		{
			switch (command.kind)
			{
				case TcCommand::Kind::RequestValue:
					server->send_request_value(client, command.ddi, command.element);
					break;
				case TcCommand::Kind::SetValue:
					// Plain Set Value: not every client accepts Set Value and Acknowledge.
					server->send_set_value(client, command.ddi, command.element, static_cast<std::uint32_t>(command.value));
					break;
				case TcCommand::Kind::TimeInterval:
					server->send_time_interval_measurement_command(client, command.ddi, command.element, static_cast<std::uint32_t>(command.value));
					break;
				case TcCommand::Kind::ChangeThreshold:
					server->send_change_threshold_measurement_command(client, command.ddi, command.element, static_cast<std::uint32_t>(command.value));
					break;
			}
		}
	}

	void TcBridge::serviceSectionControl(std::uint64_t nowMs)
	{
		const bool connected = (planClient >= 0) && (connectedAddresses.find(static_cast<std::uint8_t>(planClient)) != connectedAddresses.end());
		const bool engage = running && connected && taskActive && autoSectionControlEnabled && sectionController.plan().supports_section_control();
		const auto transition = sectionController.set_engaged(engage, nowMs);
		if (!transition.empty())
		{
			sendTcCommands(transition);
			logs.addLine(engage ? QString("[tc-sc] Section control engaged: client %1 set to automatic.").arg(planClient)
			                    : QString("[tc-sc] Section control released: sections off, client %1 back to manual.").arg(planClient));
		}
		QString status = currentSectionControlStatus;
		if (sectionController.engaged())
		{
			sendTcCommands(sectionController.update(wantedSectionStates(sectionPoses(), nowMs), nowMs));
			const auto &commanded = sectionController.commanded();
			status = QString("Automatic: %1 of %2 sections on").arg(std::count(commanded.cbegin(), commanded.cend(), true)).arg(commanded.size());
		}
		else if (sectionController.plan().supports_section_control())
		{
			status = !autoSectionControlEnabled ? QString("Manual (automatic section control off)")
			                                     : QString("Ready: starts with the task (%1 sections)").arg(sectionController.plan().section_count());
		}
		if (status != currentSectionControlStatus)
		{
			currentSectionControlStatus = status;
			emit sectionControlChanged();
		}
	}

	std::array<double, 2> TcBridge::elementOffset(const ImplementElementState &element) const
	{
		std::array<double, 2> offset = { 0.0, 0.0 };
		bool haveX = false;
		bool haveY = false;
		const ImplementElementState *current = &element;
		for (int depth = 0; (depth < 16) && (nullptr != current) && !(haveX && haveY); ++depth)
		{
			if (!haveX && current->hasOffsetX)
			{
				offset[0] = current->localX;
				haveX = true;
			}
			if (!haveY && current->hasOffsetY)
			{
				offset[1] = current->localY;
				haveY = true;
			}
			const ImplementElementState *parent = nullptr;
			for (const auto &candidate : implementElementStates)
			{
				if ((candidate.objectId == current->parentObjectId) && (&candidate != current)) parent = &candidate;
			}
			current = parent;
		}
		if (!haveY) offset[1] = element.localY; // fallback layout for sections without geometry
		return offset;
	}

	std::array<double, 2> TcBridge::connectorOffset() const
	{
		for (const auto &element : implementElementStates)
		{
			if (element.type == static_cast<int>(isobus::task_controller_object::DeviceElementObject::Type::Connector))
			{
				return elementOffset(element);
			}
		}
		return { 0.0, 0.0 };
	}

	GroundPoint TcBridge::implementGround(const std::array<double, 2> &offset, const std::array<double, 2> &connector) const
	{
		// The implement's reference point is its hitch, where the connector sits.
		const double heading = currentImplementCourse * DegreesToRadians;
		const double forward = offset[0] - connector[0];
		const double right = offset[1] - connector[1];
		return { currentImplementX + (std::sin(heading) * forward) + (std::cos(heading) * right),
			     currentImplementZ - (std::cos(heading) * forward) + (std::sin(heading) * right) };
	}

	std::vector<TcBridge::SectionPose> TcBridge::sectionPoses() const
	{
		std::vector<SectionPose> poses;
		const auto connector = connectorOffset();
		auto place = [&](double offsetX, double offsetY, double widthM) {
			poses.push_back({ implementGround({ offsetX, offsetY }, connector), widthM });
		};

		const auto &plan = sectionController.plan();
		for (const auto &boom : plan.booms)
		{
			for (const auto sectionNumber : boom.sections)
			{
				const auto found = std::find_if(implementElementStates.cbegin(), implementElementStates.cend(),
				                                [sectionNumber](const ImplementElementState &element) { return element.element == sectionNumber; });
				if (found == implementElementStates.cend())
				{
					place(0.0, 0.0, 0.0);
					continue;
				}
				const auto offset = elementOffset(*found);
				place(offset[0], offset[1], found->width);
			}
		}
		if (poses.empty() && !implementElementStates.empty())
		{
			// No sections: the implement works as one, as wide as its widest element.
			double width = 0.0;
			for (const auto &element : implementElementStates) width = std::max(width, element.width);
			place(connector[0], connector[1], std::max(width, 1.0));
		}
		return poses;
	}

	std::vector<bool> TcBridge::appliedSectionStates() const
	{
		const auto &plan = sectionController.plan();
		if (0 == plan.section_count())
		{
			return { taskActive }; // the single implement-wide section
		}
		const bool reportsActual = std::any_of(plan.booms.cbegin(), plan.booms.cend(),
		                                       [](const BoomPlan &boom) { return !boom.actualCondensedDdis.empty(); });
		if (!reportsActual)
		{
			return sectionController.engaged() ? sectionController.commanded() : std::vector<bool>(plan.section_count(), false);
		}
		std::vector<bool> states;
		for (const auto &boom : plan.booms)
		{
			for (const auto sectionNumber : boom.sections)
			{
				const auto found = std::find_if(implementElementStates.cbegin(), implementElementStates.cend(),
				                                [sectionNumber](const ImplementElementState &element) { return element.element == sectionNumber; });
				states.push_back((found != implementElementStates.cend()) && found->active);
			}
		}
		return states;
	}

	std::vector<bool> TcBridge::wantedSectionStates(const std::vector<SectionPose> &poses, std::uint64_t nowMs) const
	{
		// Look ahead by the time the client needs to switch a section, so it switches on the edge.
		double lookAheadS = 1.0;
		for (const auto &state : implementDdiStates)
		{
			if ((state.ddi == static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SCTurnOnTime)) && state.hasValue && (state.value > 0))
			{
				lookAheadS = std::clamp(static_cast<double>(state.value) / 1000.0, 0.2, 5.0);
			}
		}
		// Each section is predicted along its own measured motion, not the implement heading.
		std::vector<SectionGround> sections;
		sections.reserve(poses.size());
		for (std::size_t i = 0; i < poses.size(); ++i)
		{
			const GroundPoint velocity = (i < sectionVelocities.size()) ? sectionVelocities[i] : GroundPoint{};
			sections.push_back({ poses[i].centre, velocity, poses[i].widthM });
		}
		return wanted_section_states(sections, lookAheadS, boundaryLocal, coverage, nowMs);
	}

	void TcBridge::extendCoveragePatch(std::size_t section, GroundPoint from, GroundPoint to, double widthM)
	{
		static constexpr std::size_t MAX_PATCHES = 30000;
		if (section >= openPatchBySection.size()) openPatchBySection.resize(section + 1, -1);
		const double courseDeg = std::atan2(to.x - from.x, -(to.z - from.z)) / DegreesToRadians;
		int &open = openPatchBySection[section];
		if (open >= 0)
		{
			auto &patch = coveragePatchList[static_cast<std::size_t>(open)];
			const double length = std::hypot(patch.end.x - patch.start.x, patch.end.z - patch.start.z);
			const double turn = std::remainder(courseDeg - patch.courseDeg, 360.0);
			// Keep one patch per straight stretch; start a new one on a turn or width change.
			if ((std::abs(turn) < 3.0) && (length < 25.0) && (std::abs(patch.widthM - widthM) < 0.01))
			{
				patch.end = to;
				if (static_cast<std::size_t>(open) < publishedPatchCount) changedPatches.insert(static_cast<std::size_t>(open));
				return;
			}
		}
		if (coveragePatchList.size() >= MAX_PATCHES)
		{
			open = -1;
			return;
		}
		coveragePatchList.push_back({ from, to, widthM, courseDeg });
		open = static_cast<int>(coveragePatchList.size()) - 1;
	}

	void TcBridge::publishCoverage(bool force)
	{
		const auto nowMs = steady_clock_ms();
		if (!force && ((nowMs - lastCoveragePublishMs) < 250)) return;
		lastCoveragePublishMs = nowMs;
		auto rowOf = [](const CoveragePatch &patch) {
			const double dx = patch.end.x - patch.start.x;
			const double dz = patch.end.z - patch.start.z;
			const double length = std::hypot(dx, dz);
			QVariantMap row;
			row["x"] = (patch.start.x + patch.end.x) / 2.0;
			row["z"] = (patch.start.z + patch.end.z) / 2.0;
			row["length"] = length;
			row["width"] = patch.widthM;
			row["course"] = (length > 0.01) ? (std::atan2(dx, -dz) / DegreesToRadians) : patch.courseDeg;
			return row;
		};
		// Only what changed goes to the views: the patches extended since the last publish and
		// the new ones. Rebuilding every patch made the 3D view recreate all of its models.
		if (coveragePatchList.size() < publishedPatchCount)
		{
			currentCoveragePatches.clear();
			coveragePatchRows.clear();
			publishedPatchCount = 0;
		}
		for (const auto index : changedPatches)
		{
			const auto row = rowOf(coveragePatchList[index]);
			currentCoveragePatches[static_cast<qsizetype>(index)] = row;
			coveragePatchRows.setRow(static_cast<int>(index), row);
		}
		changedPatches.clear();
		for (; publishedPatchCount < coveragePatchList.size(); ++publishedPatchCount)
		{
			const auto row = rowOf(coveragePatchList[publishedPatchCount]);
			currentCoveragePatches.push_back(row);
			coveragePatchRows.appendRow(row);
		}
		emit workChanged();
	}

	VariantListModel *TcBridge::coveragePatchModel()
	{
		return &coveragePatchRows;
	}

	VariantListModel *TcBridge::implementElementModel()
	{
		return &implementElementRows;
	}

	VariantListModel *TcBridge::boomLedModel()
	{
		return &boomLedRows;
	}

	QVariantList TcBridge::coveragePatches() const
	{
		return currentCoveragePatches;
	}

	bool TcBridge::autoSectionControl() const
	{
		return autoSectionControlEnabled;
	}

	QString TcBridge::sectionControlStatus() const
	{
		return currentSectionControlStatus;
	}

	void TcBridge::setAutoSectionControl(bool enabled)
	{
		if (autoSectionControlEnabled == enabled) return;
		autoSectionControlEnabled = enabled;
		logs.addLine(QString("[tc-sc] Automatic section control %1.").arg(enabled ? "on" : "off"));
		emit sectionControlChanged();
		serviceSectionControl(steady_clock_ms());
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
			renderPrescription();
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
		renderPrescription(); // the local origin moved
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
