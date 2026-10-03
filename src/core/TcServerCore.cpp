#include "TcServerCore.hpp"

#include "TcClientPlan.hpp"

#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_parameter_group_number_request_protocol.hpp"
#include "isobus/utility/system_timing.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace agisotc
{
	static std::string to_hex(std::uint64_t value, unsigned width)
	{
		std::ostringstream stream;
		stream << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(width) << value;
		return stream.str();
	}

	GuiTaskControllerServer::GuiTaskControllerServer(std::shared_ptr<isobus::InternalControlFunction> internalControlFunction,
	                                                 std::uint8_t numberBoomsSupported,
	                                                 std::uint8_t numberSectionsSupported,
	                                                 std::uint8_t numberChannelsSupportedForPositionBasedControl,
	                                                 const isobus::TaskControllerOptions &options,
	                                                 TaskControllerVersion versionToReport) :
	  TaskControllerServer(internalControlFunction,
	                       numberBoomsSupported,
	                       numberSectionsSupported,
	                       numberChannelsSupportedForPositionBasedControl,
	                       options,
	                       versionToReport)
	{
	}

	GuiTaskControllerServer::~GuiTaskControllerServer()
	{
		stop_client_recovery();
	}

	GuiTaskControllerServer::ClientRecord &GuiTaskControllerServer::touch_locked(std::shared_ptr<isobus::ControlFunction> clientControlFunction)
	{
		const auto key = reinterpret_cast<std::uintptr_t>(clientControlFunction.get());
		auto existing = records.find(key);
		if (existing == records.end())
		{
			ClientRecord record;
			record.controlFunction = clientControlFunction;
			record.snapshot.lastSeenMs = steady_clock_ms();
			existing = records.emplace(key, std::move(record)).first;
			rosterDirty = true;
			log_locked("Client discovered, NAME " + to_hex(clientControlFunction->get_NAME().get_full_name(), 16));
		}
		ClientRecord &record = existing->second;
		record.snapshot.address = clientControlFunction->get_address();
		record.snapshot.nameRaw = clientControlFunction->get_NAME().get_full_name();
		record.snapshot.functionCode = clientControlFunction->get_NAME().get_function_code();
		record.snapshot.functionInstance = clientControlFunction->get_NAME().get_function_instance();
		record.snapshot.manufacturerCode = clientControlFunction->get_NAME().get_manufacturer_code();
		record.snapshot.identityNumber = clientControlFunction->get_NAME().get_identity_number();
		record.snapshot.lastSeenMs = steady_clock_ms();
		record.snapshot.timedOut = false;
		record.snapshot.ddopSizeBytes = static_cast<std::uint32_t>(record.storedPool.size());
		auto activeClient = get_active_client(clientControlFunction);
		if (nullptr != activeClient)
		{
			record.snapshot.ddopActive = activeClient->isDDOPActive;
			record.snapshot.reportedVersion = activeClient->reportedVersion;
			record.snapshot.statusBits = activeClient->statusBitfield;
		}
		return record;
	}

	void GuiTaskControllerServer::log_locked(const std::string &line)
	{
		pendingLogs.emplace_back(line);
	}

	std::shared_ptr<isobus::task_controller_object::DeviceObject> GuiTaskControllerServer::stored_device_object(const std::vector<std::uint8_t> &pool, std::uint8_t clientVersion)
	{
		if (pool.empty()) return nullptr;
		isobus::DeviceDescriptorObjectPool parsed;
		if (0 == parse_client_pool(pool, clientVersion, parsed)) return nullptr;
		for (std::uint16_t i = 0; i < parsed.size(); ++i)
		{
			auto device = std::dynamic_pointer_cast<isobus::task_controller_object::DeviceObject>(parsed.get_object_by_index(i));
			if (nullptr != device) return device;
		}
		return nullptr;
	}

	bool GuiTaskControllerServer::activate_object_pool(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
	                                                   ObjectPoolActivationError &activationError,
	                                                   ObjectPoolErrorCodes &objectPoolError,
	                                                   std::uint16_t &parentObjectIDOfFaultyObject,
	                                                   std::uint16_t &faultyObjectID)
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto &record = touch_locked(clientControlFunction);
		const bool uploaded = record.uploadOpen;
		const auto transfers = record.transfersInUpload;
		record.uploadOpen = false;
		record.transfersInUpload = 0;
		record.snapshot.ddopActive = true;
		record.snapshot.ddopSizeBytes = static_cast<std::uint32_t>(record.storedPool.size());
		rosterDirty = true;
		activationError = ObjectPoolActivationError::NoErrors;
		objectPoolError = ObjectPoolErrorCodes::NoErrors;
		parentObjectIDOfFaultyObject = 0xFFFF;
		faultyObjectID = 0xFFFF;
		// The pool is only whole once the client activates it, so the GUI parses it now.
		pendingPoolsChanged.push_back(record.snapshot.address);
		log_locked("DDOP activated by client at address " + std::to_string(record.snapshot.address) + ": " +
		           std::to_string(record.storedPool.size()) + " bytes" +
		           (uploaded ? " in " + std::to_string(transfers) + " transfer(s)" : " (stored copy)"));
		return true;
	}

	bool GuiTaskControllerServer::change_designator(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
	                                                std::uint16_t objectIDToAlter,
	                                                const std::vector<std::uint8_t> &)
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto &record = touch_locked(clientControlFunction);
		log_locked("Designator change accepted for object " + std::to_string(objectIDToAlter) +
		           " from client at address " + std::to_string(record.snapshot.address) +
		           " (accepted, not persisted to stored copy)");
		return true;
	}

	bool GuiTaskControllerServer::deactivate_object_pool(std::shared_ptr<isobus::ControlFunction> clientControlFunction)
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto &record = touch_locked(clientControlFunction);
		record.snapshot.ddopActive = false;
		record.uploadOpen = false; // A later transfer starts a new pool; the kept bytes stay viewable.
		rosterDirty = true;
		log_locked("DDOP deactivated by client at address " + std::to_string(record.snapshot.address));
		return true;
	}

	bool GuiTaskControllerServer::delete_device_descriptor_object_pool(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
	                                                                   ObjectPoolDeletionErrors &returnedErrorCode)
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto &record = touch_locked(clientControlFunction);
		record.storedPool.clear();
		record.uploadOpen = false;
		record.snapshot.ddopActive = false;
		rosterDirty = true;
		returnedErrorCode = ObjectPoolDeletionErrors::ErrorDetailsNotAvailable;
		pendingPoolsChanged.push_back(record.snapshot.address);
		log_locked("DDOP deleted by client at address " + std::to_string(record.snapshot.address));
		return true;
	}

	bool GuiTaskControllerServer::get_is_stored_device_descriptor_object_pool_by_structure_label(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
	                                                                                             const std::vector<std::uint8_t> &structureLabel,
	                                                                                             const std::vector<std::uint8_t> &extendedStructureLabel)
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto &record = touch_locked(clientControlFunction);
		bool stored = false;
		if (const auto device = stored_device_object(record.storedPool, record.snapshot.reportedVersion))
		{
			const std::string label = device->get_structure_label();
			const bool askingForStoredLabel = std::all_of(structureLabel.begin(), structureLabel.end(),
			                                              [](std::uint8_t byte) { return 0xFF == byte; });
			if (askingForStoredLabel)
			{
				// ISO 11783-10: the request carries no label, and the TC answers with the label of
				// the pool it stores, which the client compares with its own. The stack sends back
				// the vectors it passed in, which are its own non-const locals, so they are filled
				// with the stored labels here.
				auto &reply = const_cast<std::vector<std::uint8_t> &>(structureLabel);
				reply.assign(label.begin(), label.end());
				const auto extended = device->get_extended_structure_label();
				if (!extended.empty())
				{
					const_cast<std::vector<std::uint8_t> &>(extendedStructureLabel) = extended;
				}
				stored = true;
			}
			else
			{
				// A client that sends its label is told "stored" only for that same pool: any
				// other stored pool is stale, and activating it would use the wrong objects.
				stored = std::equal(structureLabel.begin(), structureLabel.end(), label.begin(), label.end()) &&
				  (extendedStructureLabel.empty() || (extendedStructureLabel == device->get_extended_structure_label()));
			}
		}
		log_locked("Client at address " + std::to_string(record.snapshot.address) +
		           " asked for DDOP by structure label; stored=" + (stored ? "yes" : "no"));
		return stored;
	}

	bool GuiTaskControllerServer::get_is_stored_device_descriptor_object_pool_by_localization_label(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
	                                                                                                const std::array<std::uint8_t, 7> &localizationLabel)
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto &record = touch_locked(clientControlFunction);
		const auto device = stored_device_object(record.storedPool, record.snapshot.reportedVersion);
		const bool stored = (nullptr != device) && (device->get_localization_label() == localizationLabel);
		log_locked("Client at address " + std::to_string(record.snapshot.address) +
		           " asked for DDOP by localization label; stored=" + (stored ? "yes" : "no"));
		return stored;
	}

	bool GuiTaskControllerServer::get_is_enough_memory_available(std::uint32_t numberBytesRequired)
	{
		static constexpr std::uint32_t MAX_POOL_BYTES = 64 * 1024;
		std::lock_guard<std::mutex> lock(mutex);
		const bool enough = (numberBytesRequired <= MAX_POOL_BYTES);
		log_locked("Object pool transfer of " + std::to_string(numberBytesRequired) +
		           " bytes requested; memory " + (enough ? "available" : "NOT available"));
		return enough;
	}

	void GuiTaskControllerServer::identify_task_controller(std::uint8_t taskControllerNumber)
	{
		std::lock_guard<std::mutex> lock(mutex);
		identifyPending = true;
		identifyPendingNumber = taskControllerNumber;
		log_locked("Identify request received for TC number " + std::to_string(taskControllerNumber));
	}

	void GuiTaskControllerServer::on_client_timeout(std::shared_ptr<isobus::ControlFunction> clientControlFunction)
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto &record = touch_locked(clientControlFunction);
		record.snapshot.ddopActive = false;
		record.snapshot.timedOut = true;
		record.uploadOpen = false;
		rosterDirty = true;
		log_locked("Client at address " + std::to_string(record.snapshot.address) + " timed out");
	}

	void GuiTaskControllerServer::on_process_data_acknowledge(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
	                                                          std::uint16_t dataDescriptionIndex,
	                                                          std::uint16_t elementNumber,
	                                                          std::uint8_t errorCodesFromClient,
	                                                          ProcessDataCommands processDataCommand)
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto &record = touch_locked(clientControlFunction);
		log_locked("PDACK from client at address " + std::to_string(record.snapshot.address) +
		           ": DDI " + std::to_string(dataDescriptionIndex) +
		           " element " + std::to_string(elementNumber) +
		           " errors " + to_hex(errorCodesFromClient, 2) +
		           " for command " + std::to_string(static_cast<unsigned>(processDataCommand)));
		DdiTrafficEvent event;
		event.address = record.snapshot.address;
		event.ddi = dataDescriptionIndex;
		event.element = elementNumber;
		event.errorCode = errorCodesFromClient;
		event.command = static_cast<std::uint8_t>(processDataCommand);
		event.acknowledge = true;
		event.timestampMs = steady_clock_ms();
		pendingDdiTraffic.push_back(event);
	}

	bool GuiTaskControllerServer::on_value_command(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
	                                               std::uint16_t dataDescriptionIndex,
	                                               std::uint16_t elementNumber,
	                                               std::int32_t processDataValue,
	                                               std::uint8_t &errorCodes)
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto &record = touch_locked(clientControlFunction);
		ValueEvent event;
		event.address = record.snapshot.address;
		event.ddi = dataDescriptionIndex;
		event.element = elementNumber;
		event.value = processDataValue;
		event.timestampMs = steady_clock_ms();
		pendingValues.push_back(event);
		DdiTrafficEvent traffic;
		traffic.address = event.address;
		traffic.ddi = event.ddi;
		traffic.element = event.element;
		traffic.value = event.value;
		traffic.hasValue = true;
		traffic.timestampMs = event.timestampMs;
		pendingDdiTraffic.push_back(traffic);
		errorCodes = 0;
		return true;
	}

	bool GuiTaskControllerServer::store_device_descriptor_object_pool(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
	                                                                  const std::vector<std::uint8_t> &objectPoolData,
	                                                                  bool appendToPool)
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto &record = touch_locked(clientControlFunction);

		// A client may send its pool in several object pool transfers, each after a request of
		// its own that announces only that transfer's size (e.g. the device object first, then
		// the process data, then the elements). All of them make up one pool until the client
		// activates it, so they are appended; the first transfer after an activation,
		// deactivation, deletion or timeout starts a new pool. The stack's append flag cannot
		// tell these apart, as it reports false for every transfer.
		(void)appendToPool;
		if (!record.uploadOpen)
		{
			record.storedPool.clear();
			record.uploadOpen = true;
			record.transfersInUpload = 0;
		}
		record.storedPool.insert(record.storedPool.end(), objectPoolData.begin(), objectPoolData.end());
		++record.transfersInUpload;
		record.snapshot.ddopSizeBytes = static_cast<std::uint32_t>(record.storedPool.size());
		rosterDirty = true;
		log_locked("DDOP transfer " + std::to_string(record.transfersInUpload) + " (" + std::to_string(objectPoolData.size()) +
		           " bytes) from client at address " + std::to_string(record.snapshot.address) + ", " +
		           std::to_string(record.storedPool.size()) + " bytes so far");
		return true;
	}

	CoreEvents GuiTaskControllerServer::take_events()
	{
		std::lock_guard<std::mutex> lock(mutex);
		CoreEvents events;
		events.logLines.assign(pendingLogs.begin(), pendingLogs.end());
		pendingLogs.clear();
		events.values.assign(pendingValues.begin(), pendingValues.end());
		pendingValues.clear();
		events.ddiTraffic.assign(pendingDdiTraffic.begin(), pendingDdiTraffic.end());
		pendingDdiTraffic.clear();
		events.rosterChanged = rosterDirty;
		rosterDirty = false;
		events.poolsChanged = std::move(pendingPoolsChanged);
		pendingPoolsChanged.clear();
		events.identifyRequested = identifyPending;
		events.identifyNumber = identifyPendingNumber;
		identifyPending = false;
		return events;
	}

	std::vector<ClientSnapshot> GuiTaskControllerServer::clients_snapshot()
	{
		std::lock_guard<std::mutex> lock(mutex);
		std::vector<ClientSnapshot> result;
		result.reserve(records.size());
		for (auto &entry : records)
		{
			result.push_back(entry.second.snapshot);
		}
		return result;
	}

	std::vector<std::uint8_t> GuiTaskControllerServer::stored_pool(std::uint8_t address)
	{
		std::lock_guard<std::mutex> lock(mutex);
		for (auto &entry : records)
		{
			if (entry.second.snapshot.address == address)
			{
				return entry.second.storedPool;
			}
		}
		return {};
	}

	void GuiTaskControllerServer::clear_stored_pool(std::uint8_t address)
	{
		std::lock_guard<std::mutex> lock(mutex);
		for (auto &entry : records)
		{
			if (entry.second.snapshot.address == address)
			{
				entry.second.storedPool.clear();
				entry.second.uploadOpen = false;
				pendingPoolsChanged.push_back(address);
				log_locked("Stored DDOP copy discarded for client at address " + std::to_string(address));
			}
		}
	}

	std::uint8_t GuiTaskControllerServer::client_version(std::uint8_t address)
	{
		std::lock_guard<std::mutex> lock(mutex);
		for (auto &entry : records)
		{
			if (entry.second.snapshot.address == address) return entry.second.snapshot.reportedVersion;
		}
		return 0;
	}

	void GuiTaskControllerServer::start_client_recovery()
	{
		if (!recoveryStarted)
		{
			isobus::CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(
			  static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProcessData), note_process_data_sender, this);
			recoveryStarted = true;
		}
	}

	void GuiTaskControllerServer::stop_client_recovery()
	{
		if (recoveryStarted)
		{
			isobus::CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(
			  static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProcessData), note_process_data_sender, this);
			recoveryStarted = false;
		}
	}

	void GuiTaskControllerServer::note_process_data_sender(const isobus::CANMessage &message, void *parentPointer)
	{
		auto *server = static_cast<GuiTaskControllerServer *>(parentPointer);
		const auto source = message.get_source_control_function();
		if ((nullptr == server) || (nullptr == source) || (message.get_destination_control_function() != server->serverControlFunction))
		{
			return;
		}
		if (0 == message.get_data_length())
		{
			return;
		}
		const auto command = static_cast<ProcessDataCommands>(message.get_data()[0] & 0x0F);
		const bool connecting = (ProcessDataCommands::TechnicalCapabilities == command) || (ProcessDataCommands::DeviceDescriptor == command);
		const bool values = (ProcessDataCommands::Value == command) || (ProcessDataCommands::SetValueAndAcknowledge == command);
		if (!connecting && !values)
		{
			return;
		}
		std::lock_guard<std::mutex> lock(server->mutex);
		auto noted = std::find_if(server->pendingSenders.begin(), server->pendingSenders.end(), [&source](const NotedSender &sender) {
			return sender.controlFunction == source;
		});
		if (noted == server->pendingSenders.end())
		{
			server->pendingSenders.push_back({ source });
			noted = server->pendingSenders.end() - 1;
		}
		noted->connecting = noted->connecting || connecting;
		noted->values = noted->values || values;
	}

	void GuiTaskControllerServer::note_out_of_step(std::map<std::uintptr_t, OutOfStepSender> &senders,
	                                               const std::shared_ptr<isobus::ControlFunction> &controlFunction,
	                                               std::uint64_t nowMs)
	{
		auto &sender = senders[reinterpret_cast<std::uintptr_t>(controlFunction.get())];
		if (nullptr == sender.controlFunction)
		{
			sender.controlFunction = controlFunction;
			sender.firstSeenMs = nowMs;
		}
		sender.lastSeenMs = nowMs;
	}

	void GuiTaskControllerServer::recover_unknown_clients()
	{
		std::vector<NotedSender> senders;
		{
			std::lock_guard<std::mutex> lock(mutex);
			senders.swap(pendingSenders);
		}
		const std::uint64_t nowMs = steady_clock_ms();
		for (const auto &sender : senders)
		{
			const auto key = reinterpret_cast<std::uintptr_t>(sender.controlFunction.get());
			const auto client = get_active_client(sender.controlFunction);
			if (sender.connecting)
			{
				// It runs the connection procedure, so it uploads its pool if this server has none.
				poollessSenders.erase(key);
				if ((key == statusHoldKey) && (nowMs < statusHeldUntilMs))
				{
					statusHeldUntilMs = 0;
					std::lock_guard<std::mutex> lock(mutex);
					log_locked("Client at address " + std::to_string(static_cast<unsigned>(sender.controlFunction->get_address())) +
					           " connects again; the TC status message is sent again");
				}
				if (nullptr != client)
				{
					unknownSenders.erase(key);
				}
				else
				{
					note_out_of_step(unknownSenders, sender.controlFunction, nowMs);
				}
			}
			else if ((nullptr != client) && client->isDDOPActive)
			{
				poollessSenders.erase(key);
			}
			else
			{
				note_out_of_step(poollessSenders, sender.controlFunction, nowMs);
			}
		}

		for (auto it = poollessSenders.begin(); it != poollessSenders.end();)
		{
			OutOfStepSender &poolless = it->second;
			const auto client = get_active_client(poolless.controlFunction);
			if (((nullptr != client) && client->isDDOPActive) || ((nowMs - poolless.lastSeenMs) > SENDER_SILENT_MS))
			{
				it = poollessSenders.erase(it);
				continue;
			}
			const bool holdAllowed = (nowMs >= statusHeldUntilMs) &&
			  ((0 == lastStatusHoldMs) || ((nowMs - lastStatusHoldMs) >= STATUS_HOLD_REPEAT_MS));
			if (((nowMs - poolless.firstSeenMs) >= POOLLESS_MS) && holdAllowed)
			{
				statusHeldUntilMs = nowMs + STATUS_HOLD_MS;
				lastStatusHoldMs = nowMs;
				statusHoldKey = it->first;
				std::lock_guard<std::mutex> lock(mutex);
				log_locked("Client at address " + std::to_string(static_cast<unsigned>(poolless.controlFunction->get_address())) +
				           " sends process values but has no active device descriptor here (it was connected before this server started, or timed out);"
				           " the TC status message is held back for " +
				           std::to_string(STATUS_HOLD_MS / 1000) + " s, so that it connects again and uploads it");
				it = poollessSenders.erase(it);
				continue;
			}
			++it;
		}
		if (nowMs < statusHeldUntilMs)
		{
			// The stack sends its status when this is 2 s old.
			lastStatusMessageTimestamp_ms = isobus::SystemTiming::get_timestamp_ms();
		}

		for (auto it = unknownSenders.begin(); it != unknownSenders.end();)
		{
			OutOfStepSender &unknown = it->second;
			const auto address = static_cast<unsigned>(unknown.controlFunction->get_address());
			if (nullptr != get_active_client(unknown.controlFunction))
			{
				std::lock_guard<std::mutex> lock(mutex);
				log_locked("Client at address " + std::to_string(address) + " sent its working set master message and is connected again");
				it = unknownSenders.erase(it);
				continue;
			}
			if ((nowMs - unknown.lastSeenMs) > SENDER_SILENT_MS)
			{
				it = unknownSenders.erase(it);
				continue;
			}
			if ((nowMs - unknown.firstSeenMs) >= IMPLICIT_CLIENT_MS)
			{
				// It did not answer: take its process data as its working set master message, as the
				// stack does on that message (a working set of one).
				activeClients.push_back(std::make_shared<ActiveClient>(unknown.controlFunction));
				std::lock_guard<std::mutex> lock(mutex);
				log_locked("Client at address " + std::to_string(address) +
				           " did not send its working set master message; accepted as a client without it");
				it = unknownSenders.erase(it);
				continue;
			}
			if ((nowMs - unknown.lastRequestMs) >= WORKING_SET_REQUEST_MS)
			{
				const bool first = (0 == unknown.lastRequestMs);
				isobus::ParameterGroupNumberRequestProtocol::request_parameter_group_number(
				  static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::WorkingSetMaster), serverControlFunction, unknown.controlFunction);
				unknown.lastRequestMs = nowMs;
				if (first)
				{
					std::lock_guard<std::mutex> lock(mutex);
					log_locked("Client at address " + std::to_string(address) +
					           " sends process data but is not known (no working set master message since it reconnected); asked it for that message");
				}
			}
			++it;
		}
	}

	std::shared_ptr<isobus::ControlFunction> GuiTaskControllerServer::find_client(std::uint8_t address)
	{
		std::lock_guard<std::mutex> lock(mutex);
		for (auto &entry : records)
		{
			if (entry.second.snapshot.address == address)
			{
				return entry.second.controlFunction;
			}
		}
		return nullptr;
	}
} // namespace agisotc
