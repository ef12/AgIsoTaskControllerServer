//================================================================================================
/// @file GpsProvider.hpp
///
/// @brief Unified GPS provider: merges NMEA serial + ISO 11783 CAN PGNs,
//================================================================================================
#pragma once

#include "GpsTypes.hpp"
#include "NmeaParser.hpp"
#include "IsoGpsHandler.hpp"

#include <chrono>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace agisotc
{
	/// @brief GPS source selector
	enum class GpsSource
	{
		Auto,       ///< Merge NMEA + ISO, prefer ISO when available
		NmeaOnly,   ///< Serial NMEA only
		IsoOnly,    ///< CAN bus ISO PGNs only
		Simulated   ///< Built-in simulator (for testing)
	};

	/// @brief Callback when a new fused solution is available.
	using SolutionCallback = std::function<void(const GpsSolution &)>;

	/// @brief Unified GPS provider.
	class GpsProvider
	{
	public:
		GpsProvider();
		~GpsProvider();

		GpsProvider(const GpsProvider &) = delete;
		GpsProvider &operator=(const GpsProvider &) = delete;

		/// @brief Sets the GPS source mode.
		void set_source(GpsSource src);

		/// @brief Starts the provider (opens serial, starts CAN listener if ISO).
		bool start(const std::string &serialPort = "", std::uint32_t baudRate = 115200);

		/// @brief Stops the provider.
		void stop();

		/// @brief Registers a callback for new solutions.
		void on_solution(SolutionCallback cb);

		/// @brief Called periodically (e.g., from TC bridge poll) to pump CAN/serial.
		void update();

		/// @brief Gets the latest fused solution.
		GpsSolution current_solution() const;

		/// @brief Feeds a raw CAN message to the ISO GPS handler.
		void feed_can_message(std::uint32_t pgn, const std::uint8_t *data, std::uint8_t len);

		/// @brief Feeds raw NMEA text (one or more sentences).
		void feed_nmea_text(std::string_view text);

		/// @brief Enables/disables the built-in simulator.
		void set_simulated(bool enabled);

		/// @brief Sets the simulated position and motion.
		void configure_simulation(double latitudeDeg, double longitudeDeg,
		                          double speedMps, double courseDeg);

		/// @brief Changes simulated speed and course without resetting the position.
		void set_simulation_motion(double speedMps, double courseDeg);

		/// @brief Rotates and moves the simulated receiver by an exact distance.
		void nudge_simulation(double forwardMeters, double turnDegrees);

	private:
		void serial_thread_func();
		void fuse_solutions();

		mutable std::mutex mutex;
		GpsSource source = GpsSource::Auto;
		NmeaParser nmea;
		IsoGpsHandler iso;
		SolutionCallback solutionCb;
		std::thread serialThread;
		std::string serialPort;
		std::uint32_t baudRate = 115200;
		std::atomic_bool running = { false };
		bool simulate = false;
		std::uint64_t lastSimulationUpdateMs = 0;
		GpsSolution fused;
	};
} // namespace agisotc
