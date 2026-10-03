//================================================================================================
/// @file ConnectionProgress.hpp
///
/// @brief Follows implements from the moment they appear on the bus until the TC has built
/// them, for a progress display: their ISO 11783-10 start-up wait, the connection, the DDOP
/// upload and the geometry values. Fed with the frames the bus sniffer sees and with what the
/// TC learns; reports the step and a 0..1 progress of the implement furthest along. No Qt.
//================================================================================================
#pragma once

#include <cstdint>
#include <map>

namespace agisotc
{
	class ConnectionProgress
	{
	public:
		enum class Step
		{
			None,       ///< Nothing connecting.
			StartingUp, ///< On the bus, in its start-up wait before it talks to a TC.
			Connecting, ///< Talking to the TC: version, structure label, ...
			Uploading,  ///< Sending its DDOP.
			Building,   ///< Pool active; the TC reads the geometry values.
			Ready       ///< Built.
		};

		struct State
		{
			Step step = Step::None;
			int address = -1;
			double progress = 0.0; ///< 0..1 over all steps.
			std::uint32_t transferBytes = 0; ///< Size of the DDOP transfer under way.
			std::uint32_t receivedBytes = 0; ///< Bytes of it received so far.
			bool uploadSkipped = false; ///< The TC had the pool stored, so nothing was uploaded.
			int geometryReceived = 0;
			int geometryTotal = 0;
			std::uint64_t elapsedMs = 0; ///< Since the implement appeared.
		};

		/// @brief How long each step took, for an implement that got ready.
		struct Timings
		{
			int address = -1;
			std::uint64_t startUpMs = 0; ///< Address claim to its first message to the TC (0 if not seen).
			std::uint64_t connectMs = 0; ///< First message to the start of the upload (or to activation).
			std::uint64_t uploadMs = 0; ///< DDOP upload, until the pool was activated.
			std::uint64_t buildMs = 0; ///< Activation until the geometry was complete.
			std::uint64_t totalMs = 0;
			std::uint32_t uploadedBytes = 0;
		};

		/// @brief An implement's start-up from its address claim to its first message to a TC: the
		/// ISO 11783-10 start-up delay of 6 s, then up to 2 s until the next TC status message.
		static constexpr std::uint64_t EXPECTED_STARTUP_MS = 8000;
		/// @brief How long the TC waits for geometry values before it shows the implement anyway.
		static constexpr std::uint64_t GEOMETRY_WAIT_MS = 8000;
		/// @brief How long a ready implement stays reported, so a display can show it complete.
		static constexpr std::uint64_t READY_HOLD_MS = 1500;
		/// @brief A device that claimed an address but did not talk to the TC within this time is no
		/// TC client (or not one of this TC) and is no longer followed.
		static constexpr std::uint64_t STARTUP_GIVE_UP_MS = 30000;
		/// @brief A connection or upload that makes no progress for this long is no longer followed,
		/// so it cannot hold an implement back.
		static constexpr std::uint64_t STALL_GIVE_UP_MS = 20000;

		/// @brief True for a NAME of an implement: agricultural industry group, device class 2 or
		/// above (0 is non-specific, as terminals and receivers are, and 1 is the tractor).
		static bool is_implement_name(std::uint64_t name);

		void reset();
		/// @brief A device claimed an address. An implement that is new (or claims again after it
		/// got ready, as after a power cycle) is followed from its start-up.
		void on_address_claim(std::uint8_t address, std::uint64_t name, std::uint64_t nowMs);
		/// @brief A device sent the working set master message: it is an implement.
		void on_working_set_master(std::uint8_t address, std::uint64_t nowMs);
		/// @brief A frame from a device to this TC; pgn without the destination address. An upload
		/// starts with the client's request for an object pool transfer (ISO 11783-10 requires it
		/// before each transfer); other messages it sends with the transport protocol, such as
		/// designator changes after the activation, are no upload.
		void on_frame_to_tc(std::uint8_t source, std::uint32_t pgn, const std::uint8_t *data, std::uint8_t length, std::uint64_t nowMs);
		/// @brief The client activated its pool.
		void on_pool_activated(std::uint8_t address, std::uint64_t nowMs);
		/// @brief How many of the client's geometry values the TC has; all of them make it ready.
		void on_geometry(std::uint8_t address, int received, int total, std::uint64_t nowMs);
		/// @brief The client timed out or left: it is no longer followed.
		void on_client_lost(std::uint8_t address);

		/// @brief Moves on what waited too long and returns the implement furthest along that is not
		/// ready, or, for READY_HOLD_MS, the one that just got ready.
		State current(std::uint64_t nowMs);
		/// @brief True once the client is built, and for a client this did not follow.
		bool is_ready(std::uint8_t address) const;
		/// @brief Returns the timings of an implement that got ready since the last call, once.
		bool take_completed(Timings &timings);

	private:
		struct Entry
		{
			Step step = Step::None;
			std::uint64_t appearedMs = 0;
			std::uint64_t startUpSeenMs = 0; ///< When its start-up was seen, 0 if it was not.
			std::uint64_t connectingMs = 0;
			std::uint64_t uploadingMs = 0;
			std::uint64_t buildingMs = 0;
			std::uint64_t readyMs = 0;
			std::uint64_t progressMs = 0; ///< When it last moved on (step or upload bytes).
			bool transferRequested = false; ///< Asked for an object pool transfer; its transport session is next.
			bool transferOpen = false; ///< The transport session of the object pool transfer is under way.
			std::uint32_t transferBytes = 0;
			std::uint32_t receivedBytes = 0;
			std::uint32_t uploadedBytes = 0; ///< All transfers together.
			int geometryReceived = 0;
			int geometryTotal = -1; ///< -1 until the TC knows.
			bool completionTaken = false;
		};

		Entry &start(std::uint8_t address, std::uint64_t nowMs);
		static void advance(Entry &entry, Step step, std::uint64_t nowMs);
		static double progress_of(const Entry &entry, std::uint64_t nowMs);

		std::map<std::uint8_t, Entry> entries;
	};
} // namespace agisotc
