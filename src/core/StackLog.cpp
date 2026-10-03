#include "StackLog.hpp"

namespace agisotc
{
	void StackLog::sink_CAN_stack_log(LoggingLevel level, const std::string &logText)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (lines.size() >= MAX_LINES)
		{
			lines.pop_front();
		}
		lines.push_back({ level, logText });
	}

	std::vector<StackLog::Line> StackLog::take_lines()
	{
		std::lock_guard<std::mutex> lock(mutex);
		std::vector<Line> taken(lines.begin(), lines.end());
		lines.clear();
		return taken;
	}
} // namespace agisotc
