//================================================================================================
/// @file PcanVirtualNetwork.hpp
///
/// @brief Registers a named PEAK PCAN Virtual (CAN-API 2) network that outlives this process.
//================================================================================================
#pragma once

#include <cstdint>
#include <string>

namespace agisotc
{
	/// @brief Makes sure a named network exists on the PEAK PCAN Virtual driver.
	/// @details Uses the installed CanApi2.dll the same way AgIsoVirtualTerminal does. A network
	/// created here stays registered after this process exits, so the TC server, the VT, and an
	/// implement simulator that only joins existing networks can be started in any order.
	/// @param[in] netName CAN-API 2 network name (1..20 bytes).
	/// @param[in] bitrate Bitrate of a newly created network, in bits per second.
	/// @param[in] preferredNetHandle Network handle (1..32) tried first when creating the network.
	/// @param[out] error Reason for a failure.
	/// @returns true if the network already existed or was created, otherwise false.
	bool ensure_pcan_virtual_network(const std::string &netName,
	                                 std::uint32_t bitrate,
	                                 std::uint8_t preferredNetHandle,
	                                 std::string &error);
} // namespace agisotc
