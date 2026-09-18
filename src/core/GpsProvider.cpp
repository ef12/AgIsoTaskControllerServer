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
#ifndef NOMINMAX
#define NOMINMAX
#endif
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
		constexpr double EarthRadiusM = 6371000.0;
		constexpr double DegreesToRadians = M_PI / 180.0;
		constexpr double RadiansToDegrees = 180.0 / M_PI;

		void move_solution(GpsSolution &solution, double distanceM)
		{
			if (!solution.latitudeDeg || !solution.longitudeDeg || !solution.courseDeg)
			{
				return;
			}
			const double headingRad = *solution.courseDeg * DegreesToRadians;
			const double latitudeRad = *solution.latitudeDeg * DegreesToRadians;
			*solution.latitudeDeg += (distanceM * std::cos(headingRad) / EarthRadiusM) * RadiansToDegrees;
			const double longitudeScale = std::max(0.01, std::cos(latitudeRad));
			*solution.longitudeDeg += (distanceM * std::sin(headingRad) /
			                           (EarthRadiusM * longitudeScale)) * RadiansToDegrees;
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
		if (source == GpsSource::Simulated)
		{
			simulate = true;
			running = true;
			return true;
		}

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
			simulate = false;
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
			if (!fused.valid)
			{
				fused.latitudeDeg = 52.0;
				fused.longitudeDeg = 5.0;
				fused.altitudeM = 10.0;
				fused.speedMps = 0.0;
				fused.courseDeg = 0.0;
			}
			if (0 != lastSimulationUpdateMs)
			{
				const double elapsedSeconds = std::min(1.0, static_cast<double>(nowMs - lastSimulationUpdateMs) / 1000.0);
				move_solution(fused, fused.speedMps.value_or(0.0) * elapsedSeconds);
			}
			lastSimulationUpdateMs = nowMs;
			fused.timestampMs = nowMs;
			fused.fixQuality = FixQuality::Simulation;
			fused.satellites = 12;
			fused.hdop = 0.8;
			fused.valid = true;
			if (solutionCb)
				solutionCb(fused);
		}
	}

	void GpsProvider::feed_can_message(std::uint32_t pgn, const std::uint8_t *data, std::uint8_t len)
	{
		std::lock_guard<std::mutex> lock(mutex);
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
		std::lock_guard<std::mutex> lock(mutex);
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
		{
			running = true;
			lastSimulationUpdateMs = 0;
		}
	}

	void GpsProvider::configure_simulation(double latitudeDeg, double longitudeDeg,
	                                      double speedMps, double courseDeg)
	{
		std::lock_guard<std::mutex> lock(mutex);
		source = GpsSource::Simulated;
		simulate = true;
		running = true;
		fused.latitudeDeg = std::clamp(latitudeDeg, -90.0, 90.0);
		fused.longitudeDeg = std::clamp(longitudeDeg, -180.0, 180.0);
		fused.altitudeM = 10.0;
		fused.speedMps = std::max(0.0, speedMps);
		fused.courseDeg = std::fmod(courseDeg + 360.0, 360.0);
		fused.fixQuality = FixQuality::Simulation;
		fused.satellites = 12;
		fused.hdop = 0.8;
		fused.valid = true;
		lastSimulationUpdateMs = 0;
	}

	void GpsProvider::set_simulation_motion(double speedMps, double courseDeg)
	{
		std::lock_guard<std::mutex> lock(mutex);
		fused.speedMps = std::max(0.0, speedMps);
		fused.courseDeg = std::fmod(courseDeg + 360.0, 360.0);
	}

	void GpsProvider::nudge_simulation(double forwardMeters, double turnDegrees)
	{
		std::lock_guard<std::mutex> lock(mutex);
		const double currentCourse = fused.courseDeg.value_or(0.0);
		fused.courseDeg = std::fmod(currentCourse + turnDegrees + 360.0, 360.0);
		move_solution(fused, forwardMeters);
		fused.valid = fused.latitudeDeg && fused.longitudeDeg;
		lastSimulationUpdateMs = 0;
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
					{
						std::lock_guard<std::mutex> lock(mutex);
						nmea.parse_sentence(line);
					}
					lineBuffer.erase(0, pos + 1);
				}
				{
					std::lock_guard<std::mutex> lock(mutex);
					fuse_solutions();
				}
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
					{
						std::lock_guard<std::mutex> lock(mutex);
						nmea.parse_sentence(line);
					}
					lineBuffer.erase(0, pos + 1);
				}
				{
					std::lock_guard<std::mutex> lock(mutex);
					fuse_solutions();
				}
			}
		}
		close(fd);
#endif
	}
} // namespace agisotc
