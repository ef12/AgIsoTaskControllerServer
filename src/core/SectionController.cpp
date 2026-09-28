#include "SectionController.hpp"

#include "isobus/isobus/isobus_standard_data_description_indices.hpp"

namespace agisotc
{
	void SectionController::reset(const ClientPlan &plan)
	{
		currentPlan = plan;
		isEngaged = false;
		lastCommanded.assign(plan.section_count(), false);
		lastSent.clear();
		for (const auto &boom : currentPlan.booms)
		{
			lastSent.emplace_back(boom.setpointCondensedDdis.size(), -1);
		}
		lastSentMs = 0;
	}

	std::vector<TcCommand> SectionController::set_engaged(bool engaged, std::uint64_t nowMs)
	{
		std::vector<TcCommand> commands;
		if ((engaged == isEngaged) || !currentPlan.supports_section_control()) return commands;
		const auto sectionControlState = static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SectionControlState);
		if (engaged)
		{
			isEngaged = true;
			if (currentPlan.sectionControlStateElement)
			{
				commands.push_back({ TcCommand::Kind::SetValue, sectionControlState, *currentPlan.sectionControlStateElement, 1 });
			}
			for (auto &groups : lastSent) groups.assign(groups.size(), -1); // send every group again
		}
		else
		{
			commands = setpoint_commands(std::vector<bool>(currentPlan.section_count(), false), true);
			if (currentPlan.sectionControlStateElement)
			{
				commands.push_back({ TcCommand::Kind::SetValue, sectionControlState, *currentPlan.sectionControlStateElement, 0 });
			}
			isEngaged = false;
		}
		lastSentMs = nowMs;
		return commands;
	}

	std::vector<TcCommand> SectionController::update(const std::vector<bool> &wanted, std::uint64_t nowMs)
	{
		if (!isEngaged) return {};
		const bool heartbeat = (nowMs - lastSentMs) >= HEARTBEAT_MS;
		auto commands = setpoint_commands(wanted, heartbeat);
		if (!commands.empty()) lastSentMs = nowMs;
		return commands;
	}

	std::vector<TcCommand> SectionController::setpoint_commands(const std::vector<bool> &wanted, bool all)
	{
		std::vector<TcCommand> commands;
		std::size_t offset = 0; // first section of the current boom in `wanted`
		for (std::size_t b = 0; b < currentPlan.booms.size(); ++b)
		{
			const auto &boom = currentPlan.booms[b];
			std::vector<bool> boomStates(boom.sections.size(), false);
			for (std::size_t s = 0; s < boom.sections.size(); ++s)
			{
				const std::size_t index = offset + s;
				boomStates[s] = (index < wanted.size()) && wanted[index];
				if (index < lastCommanded.size()) lastCommanded[index] = boomStates[s];
			}
			const auto firstDdi = static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState1_16);
			for (std::size_t g = 0; g < boom.setpointCondensedDdis.size(); ++g)
			{
				const auto ddi = boom.setpointCondensedDdis[g];
				const std::size_t firstSection = static_cast<std::size_t>(ddi - firstDdi) * 16;
				if (firstSection >= boom.sections.size()) continue; // group beyond the boom's sections
				const auto value = static_cast<std::int64_t>(encode_condensed_work_state(boomStates, firstSection));
				if (all || (lastSent[b][g] != value))
				{
					commands.push_back({ TcCommand::Kind::SetValue, ddi, boom.element, static_cast<std::int32_t>(value) });
					lastSent[b][g] = value;
				}
			}
			offset += boom.sections.size();
		}
		return commands;
	}

	bool SectionController::engaged() const
	{
		return isEngaged;
	}

	const ClientPlan &SectionController::plan() const
	{
		return currentPlan;
	}

	const std::vector<bool> &SectionController::commanded() const
	{
		return lastCommanded;
	}
} // namespace agisotc
