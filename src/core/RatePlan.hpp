//================================================================================================
/// @file RatePlan.hpp
///
/// @brief TC-GEO position-based control of a client, derived from its DDOP (ISO 11783-10 F.3.4):
/// the control channels (device elements with a settable Prescription Control State), each
/// channel's setpoints grouped per DDI and bin, and the sub-boom and section setpoints of a
/// multi-rate device below a channel. Also the command side: the Prescription Control State
/// and the setpoints a TC sends. No Qt or bus dependency.
//================================================================================================
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "PrescriptionMap.hpp"
#include "TcClientPlan.hpp"

namespace isobus
{
	class DeviceDescriptorObjectPool;
}

namespace agisotc
{
	/// @brief One settable setpoint the TC can command (a rate, or another setpoint a map holds).
	struct RateTarget
	{
		std::uint16_t ddi = 0;
		std::uint16_t element = 0;
		std::string name; ///< Designator of the element.
		/// This element and the elements above it, up to the device: where the values it inherits
		/// (latency, cultural practice) and its position come from.
		std::vector<std::uint16_t> chain;
		/// On a sub-boom or section below its channel's element: one of the rate controllers of a
		/// multi-rate device (F.24, F.25), applied at its own position.
		bool sub = false;
	};

	/// @brief The setpoints of one DDI (and one bin) in a channel: the channel-level one, which a
	/// device forwards to its sub-booms, and the sub-boom or section ones.
	struct RateGroup
	{
		std::size_t channel = 0; ///< Index into RatePlan::channels.
		std::uint16_t ddi = 0;
		std::optional<std::uint16_t> bin; ///< The bin element the product comes from, if any.
		std::optional<std::size_t> top; ///< Index into RatePlan::targets.
		std::vector<std::size_t> subs; ///< Indexes into RatePlan::targets, in element number order.

		/// @brief The targets a map is evaluated for: each sub-boom or section at its own position,
		/// else the channel-level setpoint.
		std::vector<std::size_t> map_targets() const;
		/// @brief The targets a fixed rate goes to: the channel-level setpoint (the device forwards
		/// it), else every sub-boom or section.
		std::vector<std::size_t> fixed_targets() const;
	};

	/// @brief A position-based control channel: a device element that references the
	/// Prescription Control State (ISO 11783-10 B.5.3, F.3.4.4).
	struct RateChannel
	{
		std::uint16_t element = 0;
		std::string name;
		/// False for setpoints without a Prescription Control State above them (older clients):
		/// they are commanded without one.
		bool hasPrescriptionControlState = false;
		std::vector<std::size_t> groups; ///< Indexes into RatePlan::groups.
	};

	struct RatePlan
	{
		std::vector<RateChannel> channels;
		std::vector<RateGroup> groups;
		std::vector<RateTarget> targets;
		/// Commands to send once the pool is active: requests for the latency, cultural practice
		/// and element type instance values held as process data, and a time interval
		/// measurement of the actual rates of the setpoints.
		std::vector<TcCommand> setupCommands;
		/// Device properties of the DDIs the plan inherits along an element chain, by (element, DDI).
		std::map<std::pair<std::uint16_t, std::uint16_t>, std::int32_t> properties;

		/// @brief The channels with a Prescription Control State: what the client counts against
		/// the TC's number of position-based control channels.
		std::size_t control_channel_count() const;
		/// @brief Index of the channel and group of a target.
		std::optional<std::size_t> group_of(std::size_t target) const;
	};

	/// @brief Interval of the actual rate measurement the plan sets up, in ms.
	constexpr std::uint32_t ACTUAL_RATE_INTERVAL_MS = 1000;

	/// @brief True for the application rate setpoint DDIs (ISO 11783-11).
	bool is_rate_setpoint_ddi(std::uint16_t ddi);
	/// @brief The actual rate DDI that reports a rate setpoint DDI, 0 if none.
	std::uint16_t actual_rate_ddi(std::uint16_t setpointDdi);

	/// @brief Derives the position-based control of a client from its parsed DDOP.
	/// @param[in] extraSetpointDdis Settable DDIs to control besides the application rates, for
	/// example those a prescription holds (a working depth).
	RatePlan build_rate_plan(isobus::DeviceDescriptorObjectPool &pool, const std::vector<std::uint16_t> &extraSetpointDdis = {});

	/// @brief The process data values received from the client, by element and DDI.
	using ElementValueLookup = std::function<std::optional<std::int32_t>(std::uint16_t element, std::uint16_t ddi)>;

	/// @brief The value of a DDI for an element or the nearest element above it that has one:
	/// a received process data value first, else a device property.
	std::optional<std::int32_t> inherited_value(const RatePlan &plan,
	                                            const std::vector<std::uint16_t> &chain,
	                                            std::uint16_t ddi,
	                                            const ElementValueLookup &received);

	/// @brief Picks the prescription layer for a group of setpoints (ISO 11783-10 D.41, F.3.4.7):
	/// the same DDI, and the same cultural practice and element type instance where both the
	/// layer and the device give one. The layer that matches on most of them wins.
	std::optional<std::size_t> match_layer(const std::vector<PrescriptionLayer> &layers,
	                                       std::uint16_t ddi,
	                                       std::optional<std::int32_t> culturalPractice,
	                                       std::optional<std::int32_t> elementTypeInstance);

	/// @brief TC-GEO command side: the Prescription Control State per channel and the setpoints,
	/// as ISO 11783-10 F.3.4.4 orders them.
	class RateController
	{
	public:
		/// A setpoint is sent again at this interval, even when unchanged.
		static constexpr std::uint64_t HEARTBEAT_MS = 2000;
		/// A changed setpoint waits at least this long after the last one for the same target.
		static constexpr std::uint64_t MIN_INTERVAL_MS = 250;

		/// @brief Starts over for a client's plan, all channels released.
		void reset(const RatePlan &plan);

		/// @brief Engages or releases channels.
		/// @returns Prescription Control State automatic (1) for each channel engaged (before any
		/// of its setpoints, which then all go out again: the client resets them when the state
		/// goes on), and manual (0) for each channel released.
		std::vector<TcCommand> set_engaged(const std::vector<bool> &engaged, std::uint64_t nowMs);

		/// @brief Sends automatic again to the engaged channels, with their setpoints after it.
		/// For a client that only takes the state once it sees the task active.
		std::vector<TcCommand> reassert(std::uint64_t nowMs);

		/// @brief Setpoint commands for the wanted values of the engaged channels' targets.
		/// @param[in] wanted Per target of the plan; no value sends nothing (the client keeps
		/// its setpoint).
		std::vector<TcCommand> update(const std::vector<std::optional<std::int32_t>> &wanted, std::uint64_t nowMs);

		bool engaged(std::size_t channel) const;
		bool any_engaged() const;
		const RatePlan &plan() const;
		/// @brief The setpoint last sent per target.
		const std::vector<std::optional<std::int32_t>> &commanded() const;

	private:
		std::vector<TcCommand> state_commands(std::size_t channel, bool automatic);

		RatePlan currentPlan;
		std::vector<std::size_t> channelOfTarget;
		std::vector<bool> channelEngaged;
		std::vector<std::optional<std::int32_t>> lastSent;
		std::vector<std::uint64_t> lastSentMs;
	};
} // namespace agisotc
