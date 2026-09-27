#include "PcanVirtualNetwork.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <array>
#include <sstream>
#include <vector>

namespace
{
	constexpr std::uint32_t CAN_API2_ERROR_OK = 0x0000;
	constexpr std::uint32_t CAN_API2_ERROR_INVALID_NET = 0x1800;
	constexpr std::size_t CAN_API2_MAX_NAME_LENGTH = 20;
	constexpr std::uint8_t CAN_API2_MIN_NET_HANDLE = 1;
	constexpr std::uint8_t CAN_API2_MAX_NET_HANDLE = 32;

	using SetDeviceNameFunction = std::uint32_t(WINAPI *)(char *);
	using GetErrorTextFunction = std::uint32_t(WINAPI *)(std::uint32_t, char *);
	using RegisterNetFunction = std::uint32_t(WINAPI *)(std::uint8_t, const char *, std::uint8_t, std::uint16_t);
	using RemoveNetFunction = std::uint32_t(WINAPI *)(std::uint8_t);
	using RegisterClientFunction = std::uint32_t(WINAPI *)(const char *, std::uint32_t, std::uint8_t *);
	using RemoveClientFunction = std::uint32_t(WINAPI *)(std::uint8_t);
	using ConnectToNetFunction = std::uint32_t(WINAPI *)(std::uint8_t, char *, std::uint8_t *);
	using DisconnectFromNetFunction = std::uint32_t(WINAPI *)(std::uint8_t, std::uint8_t);

	bool bitrate_to_btr0_btr1(std::uint32_t bitrate, std::uint16_t &result)
	{
		switch (bitrate)
		{
			case 1000000:
				result = 0x0014;
				break;
			case 500000:
				result = 0x001C;
				break;
			case 250000:
				result = 0x011C;
				break;
			case 125000:
				result = 0x031C;
				break;
			case 100000:
				result = 0x432F;
				break;
			case 50000:
				result = 0x472F;
				break;
			case 20000:
				result = 0x532F;
				break;
			case 10000:
				result = 0x672F;
				break;
			case 5000:
				result = 0x7F7F;
				break;
			default:
				return false;
		}
		return true;
	}

	std::string describe_windows_error(DWORD error)
	{
		LPSTR rawMessage = nullptr;
		const DWORD length = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		                                      FORMAT_MESSAGE_FROM_SYSTEM |
		                                      FORMAT_MESSAGE_IGNORE_INSERTS,
		                                    nullptr,
		                                    error,
		                                    0,
		                                    reinterpret_cast<LPSTR>(&rawMessage),
		                                    0,
		                                    nullptr);
		std::string text;
		if ((0 != length) && (nullptr != rawMessage))
		{
			text.assign(rawMessage, length);
			while ((!text.empty()) &&
			       ((text.back() == '\r') || (text.back() == '\n') || (text.back() == ' ')))
			{
				text.pop_back();
			}
		}
		if (nullptr != rawMessage)
		{
			LocalFree(rawMessage);
		}
		return "Windows error " + std::to_string(error) + (text.empty() ? "" : ": " + text);
	}

	/// @brief Releases the configuration client, its connection, and the runtime library on scope exit.
	struct CanApi2Session
	{
		HMODULE library = nullptr;
		RemoveClientFunction removeClient = nullptr;
		DisconnectFromNetFunction disconnectFromNet = nullptr;
		std::uint8_t clientHandle = 0;
		std::uint8_t netHandle = 0;
		bool clientRegistered = false;

		~CanApi2Session()
		{
			if (0 != netHandle)
			{
				disconnectFromNet(clientHandle, netHandle);
			}
			if (clientRegistered)
			{
				removeClient(clientHandle);
			}
			if (nullptr != library)
			{
				FreeLibrary(library);
			}
		}
	};
} // namespace

namespace agisotc
{
	bool ensure_pcan_virtual_network(const std::string &netName,
	                                 std::uint32_t bitrate,
	                                 std::uint8_t preferredNetHandle,
	                                 std::string &error)
	{
		if (netName.empty() ||
		    (netName.size() > CAN_API2_MAX_NAME_LENGTH) ||
		    (std::string::npos != netName.find('\0')))
		{
			error = "The PCAN Virtual network name must contain between 1 and 20 bytes.";
			return false;
		}

		std::uint16_t encodedBitrate = 0;
		if (!bitrate_to_btr0_btr1(bitrate, encodedBitrate))
		{
			error = "The requested PCAN Virtual network bitrate is not supported.";
			return false;
		}
		if ((preferredNetHandle < CAN_API2_MIN_NET_HANDLE) ||
		    (preferredNetHandle > CAN_API2_MAX_NET_HANDLE))
		{
			error = "The preferred PCAN Virtual network handle must be in the range 1..32.";
			return false;
		}

		// Only the runtime installed by the PEAK driver matches the driver service, so never
		// pick up a copy that happens to sit next to the executable.
		std::array<wchar_t, MAX_PATH> systemDirectory{};
		const UINT systemDirectoryLength = GetSystemDirectoryW(systemDirectory.data(), static_cast<UINT>(systemDirectory.size()));
		if ((0 == systemDirectoryLength) || (systemDirectoryLength >= systemDirectory.size()))
		{
			error = "Could not locate the Windows system directory (" + describe_windows_error(GetLastError()) + ").";
			return false;
		}

		CanApi2Session session;
		const std::wstring libraryPath = std::wstring(systemDirectory.data(), systemDirectoryLength) + L"\\CanApi2.dll";
		session.library = LoadLibraryW(libraryPath.c_str());
		if (nullptr == session.library)
		{
			error = "Could not load the installed CanApi2.dll (" + describe_windows_error(GetLastError()) +
			  "). Install the PEAK CAN-API 2 driver to use PCAN Virtual.";
			return false;
		}

		const auto setDeviceName = reinterpret_cast<SetDeviceNameFunction>(GetProcAddress(session.library, "CAN_SetDeviceName"));
		const auto getErrorText = reinterpret_cast<GetErrorTextFunction>(GetProcAddress(session.library, "CAN_GetErrText"));
		const auto registerNet = reinterpret_cast<RegisterNetFunction>(GetProcAddress(session.library, "CAN_RegisterNet"));
		const auto removeNet = reinterpret_cast<RemoveNetFunction>(GetProcAddress(session.library, "CAN_RemoveNet"));
		const auto registerClient = reinterpret_cast<RegisterClientFunction>(GetProcAddress(session.library, "CAN_RegisterClient"));
		const auto connectToNet = reinterpret_cast<ConnectToNetFunction>(GetProcAddress(session.library, "CAN_ConnectToNet"));
		session.removeClient = reinterpret_cast<RemoveClientFunction>(GetProcAddress(session.library, "CAN_RemoveClient"));
		session.disconnectFromNet = reinterpret_cast<DisconnectFromNetFunction>(GetProcAddress(session.library, "CAN_DisconnectFromNet"));
		if ((nullptr == setDeviceName) ||
		    (nullptr == getErrorText) ||
		    (nullptr == registerNet) ||
		    (nullptr == removeNet) ||
		    (nullptr == registerClient) ||
		    (nullptr == connectToNet) ||
		    (nullptr == session.removeClient) ||
		    (nullptr == session.disconnectFromNet))
		{
			error = "The installed CanApi2.dll does not provide all functions needed to register a PCAN Virtual network.";
			return false;
		}

		auto describeStatus = [getErrorText](std::uint32_t status) {
			std::ostringstream message;
			message << "CAN-API 2 error 0x" << std::hex << std::uppercase << status;
			std::array<char, 256> errorText{};
			if (CAN_API2_ERROR_OK == getErrorText(status, errorText.data()))
			{
				message << ": " << errorText.data();
			}
			return message.str();
		};

		char virtualDeviceName[] = "pcan_virtual";
		std::uint32_t status = setDeviceName(virtualDeviceName);
		if (CAN_API2_ERROR_OK != status)
		{
			error = "Could not select the PCAN Virtual driver: " + describeStatus(status);
			return false;
		}

		// A separate, short-lived client so the network is not tied to the CAN plugin's client.
		const std::string configurationClientName = "AgIsoTC" + std::to_string(GetCurrentProcessId());
		status = registerClient(configurationClientName.c_str(), 0, &session.clientHandle);
		if (CAN_API2_ERROR_OK != status)
		{
			error = "Could not register the PCAN Virtual configuration client: " + describeStatus(status);
			return false;
		}
		session.clientRegistered = true;

		auto connect = [&]() {
			session.netHandle = 0;
			std::vector<char> writableName(netName.begin(), netName.end());
			writableName.push_back('\0');
			return connectToNet(session.clientHandle, writableName.data(), &session.netHandle);
		};

		status = connect();
		if (CAN_API2_ERROR_OK == status)
		{
			return true;
		}
		if (CAN_API2_ERROR_INVALID_NET != status)
		{
			error = "Could not check PCAN Virtual network '" + netName + "': " + describeStatus(status);
			return false;
		}

		std::vector<std::uint8_t> handles;
		handles.push_back(preferredNetHandle);
		for (std::uint8_t handle = CAN_API2_MAX_NET_HANDLE; handle >= CAN_API2_MIN_NET_HANDLE; --handle)
		{
			if (handle != preferredNetHandle)
			{
				handles.push_back(handle);
			}
		}

		std::uint8_t createdNetHandle = 0;
		std::uint32_t registrationStatus = CAN_API2_ERROR_INVALID_NET;
		for (const auto handle : handles)
		{
			registrationStatus = registerNet(handle, netName.c_str(), 0, encodedBitrate);
			if (CAN_API2_ERROR_OK == registrationStatus)
			{
				createdNetHandle = handle;
				break;
			}
		}

		// Always retry the connection: another process may have registered the named
		// network between our initial lookup and the registration attempts.
		status = connect();
		if (CAN_API2_ERROR_OK != status)
		{
			if (0 != createdNetHandle)
			{
				error = "PCAN Virtual network '" + netName + "' was registered but could not be verified: " + describeStatus(status);
				removeNet(createdNetHandle);
			}
			else
			{
				error = "Could not register PCAN Virtual network '" + netName + "': " + describeStatus(registrationStatus);
			}
			return false;
		}
		return true;
	}
} // namespace agisotc
