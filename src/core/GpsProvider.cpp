//================================================================================================
/// @file GpsProvider.cpp
///
/// @brief Unified GPS provider implementation.
//================================================================================================
#include "GpsProvider.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace agisotc
{
	namespace
	{
		void simulate_solution(GpsSolution &sol, std::uint64_t nowMs)
		{
			static double phase = 0.0;
			const double centerLat = 52.0;
			const double centerLon = 5.0;
			const double radiusDeg = 0.001;
			const double periodMs = 120000.0;

			phase = std::fmod(static_cast<double>(nowMs) / periodMs * 2.0 * M_PI, 2.0 * M_PI);

			sol.timestampMs = nowMs;
			sol.latitudeDeg = centerLat + radiusDeg * std::cos(phase);
			sol.longitudeDeg = centerLon + radiusDeg * std::sin(phase);
			sol.altitudeM = 10.0;
			sol.speedMps = 2.0;
			sol.courseDeg = std::fmod(phase * 180.0 / M_PI + 90.0, 360.0);
			sol.fixQuality = FixQuality::GpsFix;
			sol.satellites = 8;
			sol.hdop = 1.2;
			sol.valid = true;
		}
	} // namespace

	GpsProvider::GpsProvider() = default;

	GpsProvider::~GpsProvider()
	{
		stop();
	}

	void GpsProvider::set_source(GpsSource src)
	{
		std::lock_guard<std::mutex> lock(mutex);
		source = src;
	}

	bool GpsProvider::start(const std::string &serialPort_, std::uint32_t baudRate_)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (running)
			return true;

		serialPort = serialPort_;
		baudRate = baudRate_;

		if (source == GpsSource::NmeaOnly || source == GpsSource::Auto)
		{
			if (!serialPort.empty())
			{
				running = true;
				serialThread = std::thread(&GpsProvider::serial_thread_func, this);
			}
		}

		return true;
	}

	void GpsProvider::stop()
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			running = false;
		}
		if (serialThread.joinable())
			serialThread.join();
	}

	void GpsProvider::on_solution(SolutionCallback cb)
	{
		std::lock_guard<std::mutex> lock(mutex);
		solutionCb = std::move(cb);
	}

	void GpsProvider::update()
	{
		std::lock_guard<std::mutex> lock(mutex);
		fuse_solutions();

		if (simulate)
		{
			auto nowMs = static_cast<std::uint64_t>(
				std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::steady_clock::now().time_since_epoch()).count());
			simulate_solution(fused, nowMs);
			if (solutionCb)
				solutionCb(fused);
		}
	}

	void GpsProvider::feed_can_message(std::uint32_t pgn, const std::uint8_t *data, std::uint8_t len)
	{
		if (source == GpsSource::IsoOnly || source == GpsSource::Auto)
		{
			if (iso.process_message(pgn, data, len))
			{
				fuse_solutions();
			}
		}
	}

	void GpsProvider::feed_nmea_text(std::string_view text)
	{
		if (source == GpsSource::NmeaOnly || source == GpsSource::Auto)
		{
			std::size_t start = 0;
			while (start < text.size())
			{
				auto end = text.find('\n', start);
				if (end == std::string_view::npos)
					end = text.size();
				std::string_view line = text.substr(start, end - start);
				if (!line.empty() && line.back() == '\r')
					line.remove_suffix(1);
				if (!line.empty())
					nmea.parse_sentence(line);
				if (end == std::string_view::npos)
					break;
				start = end + 1;
			}
			fuse_solutions();
		}
	}

	void GpsProvider::set_simulated(bool enabled)
	{
		std::lock_guard<std::mutex> lock(mutex);
		simulate = enabled;
		if (enabled)
			running = true;
	}

	GpsSolution GpsProvider::current_solution() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return fused;
	}

	void GpsProvider::fuse_solutions()
	{
		const auto &nmeaSol = nmea.last_solution();
		const auto &isoSol = iso.solution();

		if (source == GpsSource::IsoOnly)
		{
			fused = isoSol;
		}
		else if (source == GpsSource::NmeaOnly)
		{
			fused = nmeaSol;
		}
		else
		{
			if (isoSol.valid)
				fused = isoSol;
			else if (nmeaSol.valid)
				fused = nmeaSol;
		}

		fused.timestampMs = static_cast<std::uint64_t>(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now().time_since_epoch()).count());

		if (solutionCb && fused.valid)
			solutionCb(fused);
	}

	void GpsProvider::serial_thread_func()
	{
#ifdef _WIN32
		HANDLE hSerial = CreateFileA(serialPort.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hSerial == INVALID_HANDLE_VALUE)
			return;

		DCB dcb = { 0 };
		dcb.DCBlength = sizeof(dcb);
		if (!GetCommState(hSerial, &dcb))
		{
			CloseHandle(hSerial);
			return;
		}
		dcb.BaudRate = static_cast<DWORD>(baudRate);
		dcb.ByteSize = 8;
		dcb.StopBits = ONESTOPBIT;
		dcb.Parity = NOPARITY;
		SetCommState(hSerial, &dcb);

		COMMTIMEOUTS timeouts = { 0 };
		timeouts.ReadIntervalTimeout = 50;
		timeouts.ReadTotalTimeoutConstant = 50;
		timeouts.ReadTotalTimeoutMultiplier = 10;
		SetCommTimeouts(hSerial, &timeouts);

		char buffer[1024];
		DWORD bytesRead = 0;
		std::string lineBuffer;

		while (running)
		{
			if (ReadFile(hSerial, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0)
			{
				buffer[bytesRead] = '\0';
				lineBuffer.append(buffer, bytesRead);

				std::size_t pos;
				while ((pos = lineBuffer.find('\n')) != std::string::npos)
				{
					std::string line = lineBuffer.substr(0, pos);
					if (!line.empty() && line.back() == '\r')
						line.pop_back();
					if (!line.empty())
						nmea.parse_sentence(line);
					lineBuffer.erase(0, pos + 1);
				}
				fuse_solutions();
			}
		}
		CloseHandle(hSerial);
#else
		int fd = open(serialPort.c_str(), O_RDONLY | O_NOCTTY | O_SYNC);
		if (fd < 0)
			return;

		struct termios tty = { 0 };
		if (tcgetattr(fd, &tty) != 0)
		{
			close(fd);
			return;
		}
		cfsetospeed(&tty, static_cast<speed_t>(baudRate));
		cfsetispeed(&tty, static_cast<speed_t>(baudRate));
		tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
		tty.c_iflag &= ~IGNBRK;
		tty.c_lflag = 0;
		tty.c_oflag = 0;
		tty.c_cc[VMIN] = 1;
		tty.c_cc[VTIME] = 5;
		tty.c_iflag &= ~(IXON | IXOFF | IXANY);
		tty.c_cflag |= (CLOCAL | CREAD);
		tty.c_cflag &= ~(PARENB | PARODD | CSTOPB | CRTSCTS);
		tcsetattr(fd, TCSANOW, &tty);

		char buffer[1024];
		std::string lineBuffer;

		while (running)
		{
			ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
			if (n > 0)
			{
				buffer[n] = '\0';
				lineBuffer.append(buffer, n);

				std::size_t pos;
				while ((pos = lineBuffer.find('\n')) != std::string::npos)
				{
					std::string line = lineBuffer.substr(0, pos);
					if (!line.empty() && line.back() == '\r')
						line.pop_back();
					if (!line.empty())
						nmea.parse_sentence(line);
					lineBuffer.erase(0, pos + 1);
				}
				fuse_solutions();
			}
		}
		close(fd);
#endif
	}
} // namespace agisotc