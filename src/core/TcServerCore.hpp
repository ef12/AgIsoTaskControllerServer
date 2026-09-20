//================================================================================================
/// @file TcServerCore.hpp
///
/// @brief GUI-friendly wrapper around AgIsoStack++'s TaskControllerServer.
/// Records connected clients, received process data, and stored DDOPs, and
/// exposes them through a thread-safe event queue for the GUI thread to drain.
//================================================================================================
#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "isobus/isobus/isobus_task_controller_server.hpp"

namespace agisotc
{
	inline std::uint64_t steady_clock_ms()
	{
		return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
		  std::chrono::steady_clock::now().time_since_epoch())
		                                    .count());
	}

	/// @brief Point-in-time view of one connected TC client for the GUI.
	struct ClientSnapshot
	{
		std::uint8_t address = 0xFE; ///< Source address, or 0xFE if not yet claimed.
		std::uint64_t nameRaw = 0;
		std::uint8_t functionCode = 0;
		std::uint8_t functionInstance = 0;
		std::uint16_t manufacturerCode = 0;
		std::uint32_t identityNumber = 0;
		std::uint32_t ddopSizeBytes = 0;
		bool ddopActive = false;
		bool timedOut = false;
		std::uint8_t reportedVersion = 0;
		std::uint32_t statusBits = 0;
		std::uint64_t lastSeenMs = 0;
	};

	/// @brief One process data value received from a client.
	struct ValueEvent
	{
		std::uint8_t address = 0xFE;
		std::uint16_t ddi = 0;
		std::uint16_t element = 0;
		std::int32_t value = 0;
		std::uint64_t timestampMs = 0;
	};

	struct DdiTrafficEvent
	{
		std::uint8_t address = 0xFE;
		std::uint16_t ddi = 0;
		std::uint16_t element = 0;
		std::int32_t value = 0;
		std::uint8_t errorCode = 0;
		std::uint8_t command = 0;
		bool hasValue = false;
		bool acknowledge = false;
		std::uint64_t timestampMs = 0;
	};

	/// @brief Batch of events drained by the GUI thread.
	struct CoreEvents
	{
		std::vector<std::string> logLines;
		std::vector<ValueEvent> values;
		std::vector<DdiTrafficEvent> ddiTraffic;
		bool rosterChanged = false;
		std::vector<std::uint8_t> poolsChanged;
		bool identifyRequested = false;
		std::uint8_t identifyNumber = 0;
	};

	/// @brief TaskControllerServer implementation that records everything the GUI needs.
	/// All callbacks run on the thread that pumps update(); all shared state is mutex
	/// protected and drained via take_events() from the GUI thread.
	class GuiTaskControllerServer : public isobus::TaskControllerServer
	{
	public:
		GuiTaskControllerServer(std::shared_ptr<isobus::InternalControlFunction> internalControlFunction,
		                        std::uint8_t numberBoomsSupported,
		                        std::uint8_t numberSectionsSupported,
		                        std::uint8_t numberChannelsSupportedForPositionBasedControl,
		                        const isobus::TaskControllerOptions &options,
		                        TaskControllerVersion versionToReport = TaskControllerVersion::SecondPublishedEdition);

		~GuiTaskControllerServer() override = default;

		// TaskControllerServer callbacks.
		bool activate_object_pool(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
		                          ObjectPoolActivationError &activationError,
		                          ObjectPoolErrorCodes &objectPoolError,
		                          std::uint16_t &parentObjectIDOfFaultyObject,
		                          std::uint16_t &faultyObjectID) override;
		bool change_designator(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
		                       std::uint16_t objectIDToAlter,
		                       const std::vector<std::uint8_t> &designator) override;
		bool deactivate_object_pool(std::shared_ptr<isobus::ControlFunction> clientControlFunction) override;
		bool delete_device_descriptor_object_pool(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
		                                          ObjectPoolDeletionErrors &returnedErrorCode) override;
		bool get_is_stored_device_descriptor_object_pool_by_structure_label(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
		                                                                    const std::vector<std::uint8_t> &structureLabel,
		                                                                    const std::vector<std::uint8_t> &extendedStructureLabel) override;
		bool get_is_stored_device_descriptor_object_pool_by_localization_label(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
		                                                                       const std::array<std::uint8_t, 7> &localizationLabel) override;
		bool get_is_enough_memory_available(std::uint32_t numberBytesRequired) override;
		void identify_task_controller(std::uint8_t taskControllerNumber) override;
		void on_client_timeout(std::shared_ptr<isobus::ControlFunction> clientControlFunction) override;
		void on_process_data_acknowledge(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
		                                 std::uint16_t dataDescriptionIndex,
		                                 std::uint16_t elementNumber,
		                                 std::uint8_t errorCodesFromClient,
		                                 ProcessDataCommands processDataCommand) override;
		bool on_value_command(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
		                      std::uint16_t dataDescriptionIndex,
		                      std::uint16_t elementNumber,
		                      std::int32_t processDataValue,
		                      std::uint8_t &errorCodes) override;
		bool store_device_descriptor_object_pool(std::shared_ptr<isobus::ControlFunction> clientControlFunction,
		                                         const std::vector<std::uint8_t> &objectPoolData,
		                                         bool appendToPool) override;

		/// @brief Drains pending events. Call from the GUI thread.
		CoreEvents take_events();

		/// @brief Returns a snapshot of all known clients. Call from the GUI thread.
		std::vector<ClientSnapshot> clients_snapshot();

		/// @brief Returns the NVM-stored DDOP binary for a client address, or empty.
		std::vector<std::uint8_t> stored_pool(std::uint8_t address);

		/// @brief Discards the NVM-stored DDOP binary for a client address.
		void clear_stored_pool(std::uint8_t address);

		/// @brief Finds a client's control function by source address for commanding.
		std::shared_ptr<isobus::ControlFunction> find_client(std::uint8_t address);

	private:
		struct ClientRecord
		{
			std::shared_ptr<isobus::ControlFunction> controlFunction;
			ClientSnapshot snapshot;
			std::vector<std::uint8_t> storedPool;
			bool poolComplete = false; ///< True once a full multi-segment upload was reassembled.
		};

		ClientRecord &touch_locked(std::shared_ptr<isobus::ControlFunction> clientControlFunction);
		void log_locked(const std::string &line);

		std::mutex mutex;
		std::map<std::uintptr_t, ClientRecord> records; ///< Keyed by control function pointer.
		std::deque<std::string> pendingLogs;
		std::deque<ValueEvent> pendingValues;
		std::deque<DdiTrafficEvent> pendingDdiTraffic;
		std::vector<std::uint8_t> pendingPoolsChanged;
		bool rosterDirty = false;
		bool identifyPending = false;
		std::uint8_t identifyPendingNumber = 0;
	};
} // namespace agisotc
