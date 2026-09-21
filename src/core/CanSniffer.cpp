//================================================================================================
/// @file CanSniffer.cpp
///
/// @brief Promiscuous CAN decorator implementation.
//================================================================================================
#include "CanSniffer.hpp"

#include <algorithm>
#include <cstring>

namespace agisotc
{
	namespace
	{
		std::uint64_t steady_ms()
		{
			return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			  std::chrono::steady_clock::now().time_since_epoch())
			                                    .count());
		}
	} // namespace

	SniffingCANPlugin::SniffingCANPlugin(std::shared_ptr<isobus::CANHardwarePlugin> wrapped_) :
	  wrapped(std::move(wrapped_))
	{
	}

	std::string SniffingCANPlugin::get_name() const
	{
		return (nullptr != wrapped) ? wrapped->get_name() : std::string("SniffingCAN(empty)");
	}

	bool SniffingCANPlugin::get_is_valid() const
	{
		return (nullptr != wrapped) && wrapped->get_is_valid();
	}

	void SniffingCANPlugin::close()
	{
		if (nullptr != wrapped)
		{
			wrapped->close();
		}
	}

	void SniffingCANPlugin::open()
	{
		if (nullptr != wrapped)
		{
			wrapped->open();
		}
	}

	bool SniffingCANPlugin::read_frame(isobus::CANMessageFrame &canFrame)
	{
		const bool ok = (nullptr != wrapped) && wrapped->read_frame(canFrame);
		if (ok)
		{
			push(false, canFrame);
		}
		return ok;
	}

	bool SniffingCANPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
	{
		const bool ok = (nullptr != wrapped) && wrapped->write_frame(canFrame);
		if (ok)
		{
			push(true, canFrame);
		}
		return ok;
	}

	std::deque<SniffedFrame> SniffingCANPlugin::take_frames()
	{
		std::deque<SniffedFrame> frames;
		std::lock_guard<std::mutex> lock(mutex);
		frames.swap(queue);
		return frames;
	}

	void SniffingCANPlugin::push(bool outgoing, const isobus::CANMessageFrame &frame)
	{
		SniffedFrame sniffed;
		sniffed.identifier = frame.identifier;
		sniffed.length = frame.dataLength;
		sniffed.outgoing = outgoing;
		sniffed.timestampMs = steady_ms();
		std::memcpy(sniffed.data, frame.data, std::min<std::size_t>(frame.dataLength, sizeof(sniffed.data)));
		std::lock_guard<std::mutex> lock(mutex);
		queue.push_back(sniffed);
		while (queue.size() > MAX_QUEUE)
		{
			queue.pop_front();
		}
	}
} // namespace agisotc
