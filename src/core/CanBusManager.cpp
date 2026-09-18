#include "CanBusManager.hpp"

#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_NAME.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_preferred_addresses.hpp"

#ifdef ISOBUS_WCAN_AVAILABLE
#include "isobus/hardware_integration/wcan_plugin.hpp"
#endif

namespace agisotc
{
	CanBusManager::~CanBusManager()
	{
		stop();
	}

	bool CanBusManager::start(const CanBusSettings &settings, std::string &error)
	{
		if (running)
		{
			error = "CAN interface is already running.";
			return false;
		}

		if ((0 == settings.tcNumber) || (settings.tcNumber > 32))
		{
			error = "TC number must be in the range 1..32.";
			return false;
		}

		if ("wcan" == settings.driver)
		{
#if defined(ISOBUS_WCAN_AVAILABLE)
			driver = std::make_shared<isobus::WCANPlugin>(settings.channel);
			driverName = "WCAN (" + settings.channel + ")";
#else
			error = "WCAN driver is not available in this build.";
			return false;
#endif
		}
		else if ("pcan_usb" == settings.driver)
		{
#if defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE)
			driver = std::make_shared<isobus::PCANBasicWindowsPlugin>(PCAN_USBBUS1);
			driverName = "PEAK PCAN-USB channel 1";
#else
			error = "PEAK PCAN-USB driver is not available in this build.";
			return false;
#endif
		}
		else if ("socketcan" == settings.driver)
		{
#if defined(ISOBUS_SOCKETCAN_AVAILABLE)
			driver = std::make_shared<isobus::SocketCANInterface>(settings.channel);
			driverName = "SocketCAN (" + settings.channel + ")";
#else
			error = "SocketCAN driver is not available in this build.";
			return false;
#endif
		}
		else if ("virtual" == settings.driver)
		{
#if defined(ISOBUS_VIRTUALCAN_AVAILABLE)
			driver = std::make_shared<isobus::VirtualCANPlugin>(settings.channel);
			driverName = "Virtual CAN (" + settings.channel + ")";
#else
			error = "Virtual CAN driver is not available in this build.";
			return false;
#endif
		}
		else
		{
			error = "Unknown CAN driver: " + settings.driver;
			return false;
		}

		isobus::CANHardwareInterface::set_number_of_can_channels(1);
		isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, driver);

		if ((!isobus::CANHardwareInterface::start()) || (!driver->get_is_valid()))
		{
			error = "Failed to start the CAN hardware interface. The driver may be invalid or the device missing.";
			isobus::CANHardwareInterface::stop();
			driver.reset();
			return false;
		}

		isobus::NAME serverNAME(0);
		serverNAME.set_arbitrary_address_capable(true);
		serverNAME.set_industry_group(2);
		serverNAME.set_device_class(0);
		serverNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::TaskController));
		serverNAME.set_identity_number(static_cast<std::uint32_t>(1000 + settings.tcNumber));
		serverNAME.set_ecu_instance(0);
		serverNAME.set_function_instance(static_cast<std::uint8_t>(settings.tcNumber - 1)); // TC number is function instance + 1.
		serverNAME.set_device_class_instance(0);
		serverNAME.set_manufacturer_code(1407);

		internalControlFunction = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(
		  serverNAME,
		  0,
		  isobus::preferred_addresses::IndustryGroup2::TaskController_MappingComputer);

		if (nullptr == internalControlFunction)
		{
			error = "Failed to create the internal control function.";
			isobus::CANHardwareInterface::stop();
			driver.reset();
			return false;
		}

		running = true;
		return true;
	}

	void CanBusManager::stop()
	{
		internalControlFunction.reset();
		if (nullptr != driver)
		{
			isobus::CANHardwareInterface::stop();
			isobus::CANHardwareInterface::unassign_can_channel_frame_handler(0);
			driver.reset();
		}
		driverName.clear();
		running = false;
	}

	bool CanBusManager::is_running() const
	{
		return running;
	}

	std::shared_ptr<isobus::InternalControlFunction> CanBusManager::internal_control_function() const
	{
		return internalControlFunction;
	}

	std::string CanBusManager::active_driver_name() const
	{
		return driverName;
	}
} // namespace agisotc
