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

#include "isobus/isobus/isobus_device_descriptor_object_pool.hpp"
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
		std::uint32_t ddopSizeBytes = 0; ///< Size of the stored pool, all of its transfers together.
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

		~GuiTaskControllerServer() override;

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

		/// @brief TC version a client reported in its technical capabilities, 0 if unknown.
		std::uint8_t client_version(std::uint8_t address);

		/// @brief Starts (stops) noting the senders of process data, for recover_unknown_clients().
		void start_client_recovery();
		void stop_client_recovery();

		/// @brief Gets clients going again that are out of step with this server. Call from the thread
		/// that pumps update(), after it.
		/// - The stack accepts a client only after its working set master message, and forgets it
		///   after 6 s without its status. A client that connects again (after a change of its DDOP)
		///   without sending that message again is refused for ever. So a client the stack does not
		///   know that runs the connection procedure is asked for the message, and is let in without
		///   it if it does not answer.
		/// - A client that sends process values for a pool this server does not have (it was
		///   connected to this server before it restarted, within the client's 6 s time-out, or this
		///   server timed it out) never uploads again by itself. So the TC status message is held
		///   back for 7 s: the client loses the TC and connects again, with its pool.
		void recover_unknown_clients();

	private:
		struct ClientRecord
		{
			std::shared_ptr<isobus::ControlFunction> controlFunction;
			ClientSnapshot snapshot;
			std::vector<std::uint8_t> storedPool;
			/// True while a client is transferring its pool. A pool may arrive in several object
			/// pool transfers, each after its own request, and they all belong to one pool until
			/// the client activates it.
			bool uploadOpen = false;
			std::uint16_t transfersInUpload = 0;
		};

		/// @brief A sender of process data, as noted on the stack's thread.
		struct NotedSender
		{
			std::shared_ptr<isobus::ControlFunction> controlFunction;
			bool connecting = false; ///< Sent a technical capabilities or device descriptor command.
			bool values = false; ///< Sent process values.
		};

		/// @brief A client out of step with this server: one the stack does not know (yet) that runs
		/// the connection procedure, or one that sends process values without an active pool here.
		struct OutOfStepSender
		{
			std::shared_ptr<isobus::ControlFunction> controlFunction;
			std::uint64_t firstSeenMs = 0;
			std::uint64_t lastSeenMs = 0;
			std::uint64_t lastRequestMs = 0;
		};

		/// @brief Adds a sender to a map of out-of-step senders, or notes that it was seen again.
		static void note_out_of_step(std::map<std::uintptr_t, OutOfStepSender> &senders,
		                             const std::shared_ptr<isobus::ControlFunction> &controlFunction,
		                             std::uint64_t nowMs);

		/// @brief Process data callback of the stack (its thread): notes the sender.
		static void note_process_data_sender(const isobus::CANMessage &message, void *parentPointer);

		ClientRecord &touch_locked(std::shared_ptr<isobus::ControlFunction> clientControlFunction);
		void log_locked(const std::string &line);
		/// @brief Parses a stored pool with the client's TC version and returns its device object, or nullptr.
		static std::shared_ptr<isobus::task_controller_object::DeviceObject> stored_device_object(const std::vector<std::uint8_t> &pool, std::uint8_t clientVersion);

		std::mutex mutex;
		std::map<std::uintptr_t, ClientRecord> records; ///< Keyed by control function pointer.
		std::deque<std::string> pendingLogs;
		std::deque<ValueEvent> pendingValues;
		std::deque<DdiTrafficEvent> pendingDdiTraffic;
		std::vector<std::uint8_t> pendingPoolsChanged;
		bool rosterDirty = false;
		bool identifyPending = false;
		std::uint8_t identifyPendingNumber = 0;

		// Client recovery: senders noted on the stack's thread (under mutex), handled on the pump thread.
		std::vector<NotedSender> pendingSenders;
		std::map<std::uintptr_t, OutOfStepSender> unknownSenders; ///< Not known to the stack, connecting.
		std::map<std::uintptr_t, OutOfStepSender> poollessSenders; ///< Sending values without an active pool here.
		bool recoveryStarted = false;
		std::uint64_t statusHeldUntilMs = 0; ///< The TC status message is held back until then.
		std::uint64_t lastStatusHoldMs = 0;
		std::uintptr_t statusHoldKey = 0; ///< The client the status is held back for.
		static constexpr std::uint64_t WORKING_SET_REQUEST_MS = 1000; ///< Between two requests for the message.
		static constexpr std::uint64_t IMPLICIT_CLIENT_MS = 3000; ///< Without an answer, it is let in after this.
		static constexpr std::uint64_t SENDER_SILENT_MS = 10000; ///< A sender silent this long is forgotten.
		static constexpr std::uint64_t POOLLESS_MS = 2000; ///< Values without a pool this long hold the status.
		static constexpr std::uint64_t STATUS_HOLD_MS = 7000; ///< Longer than a client's 6 s TC time-out.
		static constexpr std::uint64_t STATUS_HOLD_REPEAT_MS = 30000; ///< At most one hold in this time.
	};
} // namespace agisotc
