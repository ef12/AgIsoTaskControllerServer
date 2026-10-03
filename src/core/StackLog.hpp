//================================================================================================
/// @file StackLog.hpp
///
/// @brief Collects the log of the CAN stack (AgIsoStack++'s CANStackLogger) for the GUI: the
/// stack logs from its own threads, and the GUI thread takes the lines.
//================================================================================================
#pragma once

#include <deque>
#include <mutex>
#include <string>
#include <vector>

#include "isobus/isobus/can_stack_logger.hpp"

namespace agisotc
{
	class StackLog : public isobus::CANStackLogger
	{
	public:
		struct Line
		{
			LoggingLevel level = LoggingLevel::Info;
			std::string text;
		};

		void sink_CAN_stack_log(LoggingLevel level, const std::string &logText) override;

		/// @brief Takes (and clears) the lines logged since the last call. Thread-safe.
		std::vector<Line> take_lines();

	private:
		std::mutex mutex;
		std::deque<Line> lines;
		static constexpr std::size_t MAX_LINES = 2000; ///< Oldest lines go first when nobody takes them.
	};
} // namespace agisotc
