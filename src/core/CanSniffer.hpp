//================================================================================================
/// @file CanSniffer.hpp
///
/// @brief Promiscuous CAN decorator: wraps any CAN driver and records every frame
/// in both directions (received and transmitted) for the bus monitor.
//================================================================================================
#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_message_frame.hpp"

namespace agisotc
{
	/// @brief One raw CAN frame seen on the wire.
	struct SniffedFrame
	{
		std::uint32_t identifier = 0;
		std::uint8_t data[8] = { 0 };
		std::uint8_t length = 0;
		bool outgoing = false;
		std::uint64_t timestampMs = 0;
	};

	/// @brief CANHardwarePlugin decorator that records all traffic passing through it.
	class SniffingCANPlugin : public isobus::CANHardwarePlugin
	{
	public:
		explicit SniffingCANPlugin(std::shared_ptr<isobus::CANHardwarePlugin> wrapped);

		std::string get_name() const override;
		bool get_is_valid() const override;
		void close() override;
		void open() override;
		bool read_frame(isobus::CANMessageFrame &canFrame) override;
		bool write_frame(const isobus::CANMessageFrame &canFrame) override;

		/// @brief Takes (and clears) all recorded frames. Thread-safe, call from any thread.
		std::deque<SniffedFrame> take_frames();

	private:
		void push(bool outgoing, const isobus::CANMessageFrame &frame);

		std::shared_ptr<isobus::CANHardwarePlugin> wrapped;
		mutable std::mutex mutex;
		std::deque<SniffedFrame> queue;
		static constexpr std::size_t MAX_QUEUE = 4000;
	};
} // namespace agisotc
