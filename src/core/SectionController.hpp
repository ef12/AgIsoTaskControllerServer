//================================================================================================
/// @file SectionController.hpp
///
/// @brief TC-SC command side: turns the section states the TC wants into the Section Control
/// State and Setpoint Condensed Work State commands of ISO 11783-10, per boom.
//================================================================================================
#pragma once

#include <cstdint>
#include <vector>

#include "TcClientPlan.hpp"

namespace agisotc
{
	class SectionController
	{
	public:
		/// Setpoints are sent again at this interval while engaged, even when unchanged.
		static constexpr std::uint64_t HEARTBEAT_MS = 1000;

		/// @brief Starts over for a client's plan, disengaged.
		void reset(const ClientPlan &plan);

		/// @brief Engages or disengages automatic section control.
		/// @returns The commands to send: Section Control State auto (1) on engaging; all sections
		/// off and Section Control State manual (0) on disengaging. Empty if nothing changed.
		std::vector<TcCommand> set_engaged(bool engaged, std::uint64_t nowMs);

		/// @brief Commands for the wanted section states while engaged.
		/// @param[in] wanted On/off per section, all booms in plan order (see ClientPlan::booms).
		/// @returns Setpoint commands for the groups of 16 that changed, or for all of them when
		/// the heartbeat is due. Empty while disengaged.
		std::vector<TcCommand> update(const std::vector<bool> &wanted, std::uint64_t nowMs);

		bool engaged() const;
		const ClientPlan &plan() const;
		/// @brief The section states last commanded, all booms in plan order.
		const std::vector<bool> &commanded() const;

	private:
		std::vector<TcCommand> setpoint_commands(const std::vector<bool> &wanted, bool all);

		ClientPlan currentPlan;
		bool isEngaged = false;
		std::vector<bool> lastCommanded;
		std::vector<std::vector<std::int64_t>> lastSent; ///< Per boom and group, -1 when never sent.
		std::uint64_t lastSentMs = 0;
	};
} // namespace agisotc
