//================================================================================================
/// @file CanBusManager.hpp
///
/// @brief Creates and owns the CAN driver and the TC server's internal control function.
//================================================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"

namespace agisotc
{
	struct CanBusSettings
	{
		std::string driver = "virtual"; ///< "wcan", "pcan_usb", "virtual", or "socketcan".
		std::string channel = "TC-Server"; ///< Bus name, or SocketCAN interface like "can0".
		std::uint8_t tcNumber = 1; ///< Our TC number (function instance + 1, range 1..32).
	};

	class CanBusManager
	{
	public:
		CanBusManager() = default;
		~CanBusManager();

		CanBusManager(const CanBusManager &) = delete;
		CanBusManager &operator=(const CanBusManager &) = delete;

		/// @brief Starts the CAN interface and claims our TC address.
		/// @returns true on success, otherwise false with a reason in error.
		bool start(const CanBusSettings &settings, std::string &error);

		/// @brief Stops the CAN interface.
		void stop();

		bool is_running() const;
		std::shared_ptr<isobus::InternalControlFunction> internal_control_function() const;
		std::string active_driver_name() const;

	private:
		std::shared_ptr<isobus::CANHardwarePlugin> driver;
		std::shared_ptr<isobus::InternalControlFunction> internalControlFunction;
		std::string driverName;
		bool running = false;
	};
} // namespace agisotc
