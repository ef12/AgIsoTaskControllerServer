#include "TcServerCore.hpp"

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
		auto activeClient = get_active_client(clientControlFunction);
		if (nullptr != activeClient)
		{
			record.snapshot.ddopSizeBytes = activeClient->clientDDOPsize_bytes;
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

	bool GuiTaskControllerServer::activate_object_pool(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
	                                                   ObjectPoolActivationError &activationError,
	                                                   ObjectPoolErrorCodes &objectPoolError,
	                                                   std::uint16_t &parentObjectIDOfFaultyObject,
	                                                   std::uint16_t &faultyObjectID)
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto &record = touch_locked(clientControlFunction);
		record.snapshot.ddopActive = true;
		rosterDirty = true;
		activationError = ObjectPoolActivationError::NoErrors;
		objectPoolError = ObjectPoolErrorCodes::NoErrors;
		parentObjectIDOfFaultyObject = 0xFFFF;
		faultyObjectID = 0xFFFF;
		log_locked("DDOP activated by client at address " + std::to_string(record.snapshot.address));
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
		record.poolComplete = false; // Next upload starts a fresh reassembly, kept bytes stay viewable.
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
		record.poolComplete = false;
		record.snapshot.ddopActive = false;
		rosterDirty = true;
		returnedErrorCode = ObjectPoolDeletionErrors::ErrorDetailsNotAvailable;
		pendingPoolsChanged.push_back(record.snapshot.address);
		log_locked("DDOP deleted by client at address " + std::to_string(record.snapshot.address));
		return true;
	}

	bool GuiTaskControllerServer::get_is_stored_device_descriptor_object_pool_by_structure_label(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
	                                                                                             const std::vector<std::uint8_t> &,
	                                                                                             const std::vector<std::uint8_t> &)
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto &record = touch_locked(clientControlFunction);
		// TODO: match the requested structure label against the stored pool instead of
		// reporting any stored pool. v1 keeps an in-memory NVM per client.
		const bool stored = !record.storedPool.empty();
		log_locked("Client at address " + std::to_string(record.snapshot.address) +
		           " asked for DDOP by structure label; stored=" + (stored ? "yes" : "no"));
		return stored;
	}

	bool GuiTaskControllerServer::get_is_stored_device_descriptor_object_pool_by_localization_label(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
	                                                                                                const std::array<std::uint8_t, 7> &)
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto &record = touch_locked(clientControlFunction);
		const bool stored = !record.storedPool.empty();
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
		record.poolComplete = false;
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

		// The stack reports append=false for every segment (its segment counter
		// is never incremented), so large pools would arrive here as a series
		// of overwriting chunks. Reassemble multi-segment uploads ourselves
		// using the total size the client announced in RequestObjectPoolTransfer.
		auto activeClient = get_active_client(clientControlFunction);
		const std::uint32_t expectedBytes = (nullptr != activeClient) ? activeClient->clientDDOPsize_bytes : 0;
		if (expectedBytes > 0)
		{
			const bool startOver = record.poolComplete ||
				(record.storedPool.size() + objectPoolData.size() > expectedBytes);
			if (startOver)
			{
				record.storedPool.clear();
				record.poolComplete = false;
			}
			record.storedPool.insert(record.storedPool.end(), objectPoolData.begin(), objectPoolData.end());
			record.poolComplete = (record.storedPool.size() >= expectedBytes);
		}
		else
		{
			if (!appendToPool)
			{
				record.storedPool.clear();
			}
			record.storedPool.insert(record.storedPool.end(), objectPoolData.begin(), objectPoolData.end());
			record.poolComplete = true;
		}

		if (record.poolComplete)
		{
			pendingPoolsChanged.push_back(record.snapshot.address);
		}
		log_locked("DDOP segment (" + std::to_string(objectPoolData.size()) + " bytes) from client at address " +
		           std::to_string(record.snapshot.address) + ", accumulated " +
		           std::to_string(record.storedPool.size()) + " of " + std::to_string(expectedBytes) +
		           (record.poolComplete ? " (complete)" : " (waiting for more)"));
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
				entry.second.poolComplete = false;
				pendingPoolsChanged.push_back(address);
				log_locked("Stored DDOP copy discarded for client at address " + std::to_string(address));
			}
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
