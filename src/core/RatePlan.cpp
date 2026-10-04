#include "RatePlan.hpp"

#include <algorithm>
#include <memory>
#include <tuple>

#include "isobus/isobus/isobus_device_descriptor_object_pool.hpp"
#include "isobus/isobus/isobus_standard_data_description_indices.hpp"
#include "isobus/isobus/isobus_task_controller_client_objects.hpp"

namespace agisotc
{
	namespace
	{
		using DDI = isobus::DataDescriptionIndex;
		using isobus::task_controller_object::DeviceElementObject;
		using isobus::task_controller_object::DeviceProcessDataObject;
		using isobus::task_controller_object::DevicePropertyObject;

		/// The application rate setpoints (ISO 11783-11), with the actual rate DDI of each.
		constexpr std::pair<DDI, DDI> RATE_SETPOINTS[] = {
			{ DDI::SetpointVolumePerAreaApplicationRate, DDI::ActualVolumePerAreaApplicationRate },
			{ DDI::SetpointMassPerAreaApplicationRate, DDI::ActualMassPerAreaApplicationRate },
			{ DDI::SetpointCountPerAreaApplicationRate, DDI::ActualCountPerAreaApplicationRate },
			{ DDI::SetpointSpacingApplicationRate, DDI::ActualSpacingApplicationRate },
			{ DDI::SetpointVolumePerVolumeApplicationRate, DDI::ActualVolumePerVolumeApplicationRate },
			{ DDI::SetpointMassPerMassApplicationRate, DDI::ActualMassPerMassApplicationRate },
			{ DDI::SetpointVolumePerMassApplicationRate, DDI::ActualVolumePerMassApplicationRate },
			{ DDI::SetpointVolumePerTimeApplicationRate, DDI::ActualVolumePerTimeApplicationRate },
			{ DDI::SetpointMassPerTimeApplicationRate, DDI::ActualMassPerTimeApplicationRate },
			{ DDI::SetpointCountPerTimeApplicationRate, DDI::ActualCountPerTimeApplicationRate },
			{ DDI::SetpointPercentageApplicationRate, DDI::ActualPercentageApplicationRate },
		};

		/// Settable DDIs that steer the connection itself, never taken from a map.
		bool is_control_ddi(std::uint16_t ddi)
		{
			const auto first = static_cast<std::uint16_t>(DDI::SetpointCondensedWorkState1_16);
			return (static_cast<std::uint16_t>(DDI::PrescriptionControlState) == ddi) ||
			  (static_cast<std::uint16_t>(DDI::SectionControlState) == ddi) ||
			  (static_cast<std::uint16_t>(DDI::SetpointWorkState) == ddi) ||
			  (static_cast<std::uint16_t>(DDI::RequestDefaultProcessData) == ddi) ||
			  ((ddi >= first) && (ddi < first + 16));
		}

		/// The DDIs whose values a setpoint inherits from the elements above it.
		constexpr DDI INHERITED_DDIS[] = {
			DDI::PhysicalSetpointTimeLatency,
			DDI::PhysicalActualValueTimeLatency,
			DDI::ElementTypeInstance,
			DDI::ActualCulturalPractice,
		};

		bool is_inherited_ddi(std::uint16_t ddi)
		{
			return std::any_of(std::begin(INHERITED_DDIS), std::end(INHERITED_DDIS), [ddi](DDI inherited) { return static_cast<std::uint16_t>(inherited) == ddi; });
		}

		struct ElementInfo
		{
			std::uint16_t objectId = 0;
			std::uint16_t number = 0;
			std::uint16_t parentObjectId = 0;
			DeviceElementObject::Type type = DeviceElementObject::Type::Device;
			std::string name;
			std::vector<std::shared_ptr<DeviceProcessDataObject>> processData;
			std::vector<std::shared_ptr<DevicePropertyObject>> properties;
		};
	} // namespace

	std::vector<std::size_t> RateGroup::map_targets() const
	{
		if (!subs.empty()) return subs;
		return top ? std::vector<std::size_t>{ *top } : std::vector<std::size_t>{};
	}

	std::vector<std::size_t> RateGroup::fixed_targets() const
	{
		return top ? std::vector<std::size_t>{ *top } : subs;
	}

	std::size_t RatePlan::control_channel_count() const
	{
		return static_cast<std::size_t>(std::count_if(channels.cbegin(), channels.cend(), [](const RateChannel &channel) { return channel.hasPrescriptionControlState; }));
	}

	std::optional<std::size_t> RatePlan::group_of(std::size_t target) const
	{
		for (std::size_t index = 0; index < groups.size(); ++index)
		{
			const auto &group = groups[index];
			if ((group.top && (*group.top == target)) || (std::find(group.subs.cbegin(), group.subs.cend(), target) != group.subs.cend())) return index;
		}
		return std::nullopt;
	}

	bool is_rate_setpoint_ddi(std::uint16_t ddi)
	{
		return std::any_of(std::begin(RATE_SETPOINTS), std::end(RATE_SETPOINTS), [ddi](const std::pair<DDI, DDI> &rate) { return static_cast<std::uint16_t>(rate.first) == ddi; });
	}

	std::uint16_t actual_rate_ddi(std::uint16_t setpointDdi)
	{
		for (const auto &[setpoint, actual] : RATE_SETPOINTS)
		{
			if (static_cast<std::uint16_t>(setpoint) == setpointDdi) return static_cast<std::uint16_t>(actual);
		}
		return 0;
	}

	RatePlan build_rate_plan(isobus::DeviceDescriptorObjectPool &pool, const std::vector<std::uint16_t> &extraSetpointDdis)
	{
		RatePlan plan;
		std::map<std::uint16_t, ElementInfo> elements; // by object ID
		for (std::uint16_t i = 0; i < pool.size(); ++i)
		{
			auto element = std::dynamic_pointer_cast<DeviceElementObject>(pool.get_object_by_index(i));
			if (nullptr == element) continue;
			ElementInfo info;
			info.objectId = element->get_object_id();
			info.number = element->get_element_number();
			info.parentObjectId = element->get_parent_object();
			info.type = element->get_type();
			info.name = element->get_designator();
			for (const auto childId : child_object_ids(*element))
			{
				auto child = pool.get_object_by_id(childId);
				if (auto processData = std::dynamic_pointer_cast<DeviceProcessDataObject>(child)) info.processData.push_back(processData);
				else if (auto property = std::dynamic_pointer_cast<DevicePropertyObject>(child)) info.properties.push_back(property);
			}
			elements.emplace(info.objectId, std::move(info));
		}

		// The elements from one up to the device, as object IDs.
		auto chain_of = [&elements](std::uint16_t objectId) {
			std::vector<std::uint16_t> chain;
			for (int depth = 0; depth < 32; ++depth)
			{
				const auto found = elements.find(objectId);
				if (found == elements.end()) break;
				chain.push_back(objectId);
				if (found->second.parentObjectId == objectId) break;
				objectId = found->second.parentObjectId;
			}
			return chain;
		};
		auto is_setpoint = [&extraSetpointDdis](DeviceProcessDataObject &processData) {
			const auto ddi = processData.get_ddi();
			if (!processData.has_property(DeviceProcessDataObject::PropertiesBit::Settable) || is_control_ddi(ddi)) return false;
			return is_rate_setpoint_ddi(ddi) || (std::find(extraSetpointDdis.cbegin(), extraSetpointDdis.cend(), ddi) != extraSetpointDdis.cend());
		};
		auto has_setpoint = [&](const ElementInfo &info, std::uint16_t ddi) {
			return std::any_of(info.processData.cbegin(), info.processData.cend(), [&](const std::shared_ptr<DeviceProcessDataObject> &processData) {
				return (processData->get_ddi() == ddi) && is_setpoint(*processData);
			});
		};
		auto has_state = [](const ElementInfo &info) {
			return std::any_of(info.processData.cbegin(), info.processData.cend(), [](const std::shared_ptr<DeviceProcessDataObject> &processData) {
				return (static_cast<std::uint16_t>(DDI::PrescriptionControlState) == processData->get_ddi()) &&
				  processData->has_property(DeviceProcessDataObject::PropertiesBit::Settable);
			});
		};

		struct Found
		{
			std::uint16_t channelObject = 0;
			std::uint16_t ddi = 0;
			std::optional<std::uint16_t> bin;
			bool sub = false;
			RateTarget target;
		};
		std::vector<Found> found;
		for (const auto &[objectId, info] : elements)
		{
			for (const auto &processData : info.processData)
			{
				if (!is_setpoint(*processData)) continue;
				const auto ddi = processData->get_ddi();
				const auto chain = chain_of(objectId);
				// The channel is the nearest element with a Prescription Control State. Without
				// one, the topmost element with the same setpoint stands for the channel.
				std::optional<std::size_t> channelAt;
				for (std::size_t at = 0; at < chain.size(); ++at)
				{
					if (has_state(elements.at(chain[at])))
					{
						channelAt = at;
						break;
					}
				}
				if (!channelAt)
				{
					for (std::size_t at = 0; at < chain.size(); ++at)
					{
						if (has_setpoint(elements.at(chain[at]), ddi)) channelAt = at;
					}
				}
				Found entry;
				entry.channelObject = chain[*channelAt];
				entry.ddi = ddi;
				for (std::size_t at = 0; at <= *channelAt; ++at)
				{
					const auto &link = elements.at(chain[at]);
					if ((DeviceElementObject::Type::Bin == link.type) && !entry.bin) entry.bin = link.number;
					// A setpoint on (or below) a sub-boom or section of the channel is one of its rate controllers.
					if ((at < *channelAt) && ((DeviceElementObject::Type::Function == link.type) || (DeviceElementObject::Type::Section == link.type))) entry.sub = true;
				}
				entry.target.ddi = ddi;
				entry.target.element = info.number;
				entry.target.name = info.name;
				entry.target.sub = entry.sub;
				for (const auto link : chain) entry.target.chain.push_back(elements.at(link).number);
				found.push_back(std::move(entry));
			}
		}

		std::sort(found.begin(), found.end(), [&elements](const Found &left, const Found &right) {
			const auto leftChannel = elements.at(left.channelObject).number;
			const auto rightChannel = elements.at(right.channelObject).number;
			return std::make_tuple(leftChannel, left.ddi, left.bin.value_or(0), left.sub, left.target.element) <
			  std::make_tuple(rightChannel, right.ddi, right.bin.value_or(0), right.sub, right.target.element);
		});
		for (auto &entry : found)
		{
			const auto &channelInfo = elements.at(entry.channelObject);
			auto channel = std::find_if(plan.channels.begin(), plan.channels.end(), [&](const RateChannel &existing) { return existing.element == channelInfo.number; });
			if (channel == plan.channels.end())
			{
				RateChannel fresh;
				fresh.element = channelInfo.number;
				fresh.name = channelInfo.name;
				fresh.hasPrescriptionControlState = has_state(channelInfo);
				plan.channels.push_back(fresh);
				channel = plan.channels.end() - 1;
			}
			const auto channelIndex = static_cast<std::size_t>(channel - plan.channels.begin());
			auto group = std::find_if(plan.groups.begin(), plan.groups.end(), [&](const RateGroup &existing) {
				return (existing.channel == channelIndex) && (existing.ddi == entry.ddi) && (existing.bin == entry.bin);
			});
			if (group == plan.groups.end())
			{
				RateGroup fresh;
				fresh.channel = channelIndex;
				fresh.ddi = entry.ddi;
				fresh.bin = entry.bin;
				plan.groups.push_back(fresh);
				group = plan.groups.end() - 1;
				plan.channels[channelIndex].groups.push_back(static_cast<std::size_t>(group - plan.groups.begin()));
			}
			const std::size_t targetIndex = plan.targets.size();
			if (!entry.sub && !group->top)
			{
				group->top = targetIndex;
			}
			else
			{
				entry.target.sub = true;
				group->subs.push_back(targetIndex);
			}
			plan.targets.push_back(std::move(entry.target));
		}

		// Values the setpoints inherit: properties are known now, process data is asked for.
		for (const auto &[objectId, info] : elements)
		{
			for (const auto &property : info.properties)
			{
				if (is_inherited_ddi(property->get_ddi())) plan.properties[{ info.number, property->get_ddi() }] = property->get_value();
			}
			for (const auto &processData : info.processData)
			{
				const auto ddi = processData->get_ddi();
				if (is_inherited_ddi(ddi))
				{
					plan.setupCommands.push_back({ TcCommand::Kind::RequestValue, ddi, info.number, 0 });
				}
			}
		}
		// The actual rate of each setpoint, measured at a time interval (TC-GEO, F.3.4).
		for (const auto &target : plan.targets)
		{
			const auto actual = actual_rate_ddi(target.ddi);
			if (0 == actual) continue;
			for (const auto &[objectId, info] : elements)
			{
				if (info.number != target.element) continue;
				for (const auto &processData : info.processData)
				{
					if ((processData->get_ddi() == actual) && processData->has_trigger_method(DeviceProcessDataObject::AvailableTriggerMethods::TimeInterval))
					{
						const TcCommand command{ TcCommand::Kind::TimeInterval, actual, target.element, static_cast<std::int32_t>(ACTUAL_RATE_INTERVAL_MS) };
						if (std::find(plan.setupCommands.cbegin(), plan.setupCommands.cend(), command) == plan.setupCommands.cend()) plan.setupCommands.push_back(command);
					}
				}
			}
		}
		return plan;
	}

	std::optional<std::int32_t> inherited_value(const RatePlan &plan,
	                                            const std::vector<std::uint16_t> &chain,
	                                            std::uint16_t ddi,
	                                            const ElementValueLookup &received)
	{
		for (const auto element : chain)
		{
			if (received)
			{
				if (const auto value = received(element, ddi)) return value;
			}
			const auto property = plan.properties.find({ element, ddi });
			if (property != plan.properties.end()) return property->second;
		}
		return std::nullopt;
	}

	std::optional<std::size_t> match_layer(const std::vector<PrescriptionLayer> &layers,
	                                       std::uint16_t ddi,
	                                       std::optional<std::int32_t> culturalPractice,
	                                       std::optional<std::int32_t> elementTypeInstance)
	{
		std::optional<std::size_t> best;
		int bestScore = -1;
		for (std::size_t index = 0; index < layers.size(); ++index)
		{
			const auto &layer = layers[index];
			if (layer.ddi != ddi) continue;
			int score = 0;
			if (layer.culturalPractice && culturalPractice)
			{
				if (*layer.culturalPractice != *culturalPractice) continue;
				++score;
			}
			if (layer.elementTypeInstance && elementTypeInstance)
			{
				if (*layer.elementTypeInstance != *elementTypeInstance) continue;
				++score;
			}
			if (score > bestScore)
			{
				best = index;
				bestScore = score;
			}
		}
		return best;
	}

	void RateController::reset(const RatePlan &plan)
	{
		currentPlan = plan;
		channelOfTarget.assign(plan.targets.size(), 0);
		for (const auto &group : plan.groups)
		{
			if (group.top) channelOfTarget[*group.top] = group.channel;
			for (const auto sub : group.subs) channelOfTarget[sub] = group.channel;
		}
		channelEngaged.assign(plan.channels.size(), false);
		lastSent.assign(plan.targets.size(), std::nullopt);
		lastSentMs.assign(plan.targets.size(), 0);
	}

	std::vector<TcCommand> RateController::state_commands(std::size_t channel, bool automatic)
	{
		std::vector<TcCommand> commands;
		if (currentPlan.channels[channel].hasPrescriptionControlState)
		{
			commands.push_back({ TcCommand::Kind::SetValue, static_cast<std::uint16_t>(DDI::PrescriptionControlState), currentPlan.channels[channel].element, automatic ? 1 : 0 });
		}
		// Whatever the client had, its setpoints go out again after the state.
		for (std::size_t target = 0; target < lastSent.size(); ++target)
		{
			if (channelOfTarget[target] == channel) lastSent[target].reset();
		}
		return commands;
	}

	std::vector<TcCommand> RateController::set_engaged(const std::vector<bool> &engaged, std::uint64_t)
	{
		std::vector<TcCommand> commands;
		for (std::size_t channel = 0; channel < channelEngaged.size(); ++channel)
		{
			const bool wanted = (channel < engaged.size()) && engaged[channel];
			if (wanted == channelEngaged[channel]) continue;
			channelEngaged[channel] = wanted;
			const auto state = state_commands(channel, wanted);
			commands.insert(commands.end(), state.cbegin(), state.cend());
		}
		return commands;
	}

	std::vector<TcCommand> RateController::reassert(std::uint64_t)
	{
		std::vector<TcCommand> commands;
		for (std::size_t channel = 0; channel < channelEngaged.size(); ++channel)
		{
			if (!channelEngaged[channel]) continue;
			const auto state = state_commands(channel, true);
			commands.insert(commands.end(), state.cbegin(), state.cend());
		}
		return commands;
	}

	std::vector<TcCommand> RateController::update(const std::vector<std::optional<std::int32_t>> &wanted, std::uint64_t nowMs)
	{
		std::vector<TcCommand> commands;
		for (std::size_t target = 0; (target < wanted.size()) && (target < lastSent.size()); ++target)
		{
			if (!channelEngaged[channelOfTarget[target]] || !wanted[target]) continue;
			const std::uint64_t sinceLast = nowMs - lastSentMs[target];
			const bool first = !lastSent[target];
			const bool changed = !first && (*lastSent[target] != *wanted[target]) && (sinceLast >= MIN_INTERVAL_MS);
			const bool heartbeat = !first && (sinceLast >= HEARTBEAT_MS);
			if (!first && !changed && !heartbeat) continue;
			const auto &setpoint = currentPlan.targets[target];
			commands.push_back({ TcCommand::Kind::SetValue, setpoint.ddi, setpoint.element, *wanted[target] });
			lastSent[target] = wanted[target];
			lastSentMs[target] = nowMs;
		}
		return commands;
	}

	bool RateController::engaged(std::size_t channel) const
	{
		return (channel < channelEngaged.size()) && channelEngaged[channel];
	}

	bool RateController::any_engaged() const
	{
		return std::any_of(channelEngaged.cbegin(), channelEngaged.cend(), [](bool engaged) { return engaged; });
	}

	const RatePlan &RateController::plan() const
	{
		return currentPlan;
	}

	const std::vector<std::optional<std::int32_t>> &RateController::commanded() const
	{
		return lastSent;
	}
} // namespace agisotc
