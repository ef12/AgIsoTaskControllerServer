#include "TcClientPlan.hpp"

#include <algorithm>
#include <map>
#include <memory>

#include "isobus/isobus/isobus_standard_data_description_indices.hpp"
#include "isobus/isobus/isobus_task_controller_client_objects.hpp"

namespace agisotc
{
	namespace
	{
		using DDI = isobus::DataDescriptionIndex;
		using isobus::task_controller_object::DeviceElementObject;
		using isobus::task_controller_object::DeviceProcessDataObject;

		constexpr std::uint16_t CONDENSED_GROUPS = 16; ///< 16 condensed DDIs of 16 sections each

		/// DDIs of the application rate setpoints a TC can command.
		constexpr DDI RATE_SETPOINT_DDIS[] = {
			DDI::SetpointVolumePerAreaApplicationRate,
			DDI::SetpointMassPerAreaApplicationRate,
			DDI::SetpointCountPerAreaApplicationRate,
			DDI::SetpointSpacingApplicationRate,
			DDI::SetpointVolumePerVolumeApplicationRate,
			DDI::SetpointMassPerMassApplicationRate,
			DDI::SetpointVolumePerMassApplicationRate,
			DDI::SetpointVolumePerTimeApplicationRate,
			DDI::SetpointMassPerTimeApplicationRate,
			DDI::SetpointCountPerTimeApplicationRate,
			DDI::SetpointPercentageApplicationRate,
		};

		struct ElementInfo
		{
			std::uint16_t number = 0;
			std::uint16_t parentObjectId = 0;
			DeviceElementObject::Type type = DeviceElementObject::Type::Device;
			std::vector<std::shared_ptr<DeviceProcessDataObject>> processData;
		};

		bool is_rate_setpoint(std::uint16_t ddi)
		{
			return std::any_of(std::begin(RATE_SETPOINT_DDIS), std::end(RATE_SETPOINT_DDIS),
			                   [ddi](DDI rate) { return static_cast<std::uint16_t>(rate) == ddi; });
		}
	} // namespace

	bool is_actual_condensed_work_state(std::uint16_t ddi)
	{
		const auto first = static_cast<std::uint16_t>(DDI::ActualCondensedWorkState1_16);
		return (ddi >= first) && (ddi < first + CONDENSED_GROUPS);
	}

	bool is_setpoint_condensed_work_state(std::uint16_t ddi)
	{
		const auto first = static_cast<std::uint16_t>(DDI::SetpointCondensedWorkState1_16);
		return (ddi >= first) && (ddi < first + CONDENSED_GROUPS);
	}

	std::vector<std::uint16_t> child_object_ids(isobus::task_controller_object::DeviceElementObject &element)
	{
		std::vector<std::uint16_t> ids;
		const auto count = element.get_number_child_objects();
		ids.reserve(count);
		for (std::uint16_t i = 0; i < count; ++i)
		{
			ids.push_back(element.get_child_object_id(i));
		}
		return ids;
	}

	std::uint8_t parse_client_pool(const std::vector<std::uint8_t> &binary, std::uint8_t clientVersion, isobus::DeviceDescriptorObjectPool &pool)
	{
		constexpr std::uint8_t NEWEST_DDOP_LAYOUT = 4; ///< Newest TC version the stack parses
		std::vector<std::uint8_t> versions;
		if ((clientVersion >= 2) && (clientVersion <= NEWEST_DDOP_LAYOUT))
		{
			versions.push_back(clientVersion);
		}
		for (const std::uint8_t version : { std::uint8_t{ 4 }, std::uint8_t{ 3 }, std::uint8_t{ 2 } })
		{
			if (version != clientVersion) versions.push_back(version);
		}
		for (const auto version : versions)
		{
			pool.clear();
			pool.set_task_controller_compatibility_level(version);
			std::vector<std::uint8_t> copy = binary; // the parser consumes its input
			if (!copy.empty() && pool.deserialize_binary_object_pool(copy)) return version;
		}
		pool.clear();
		return 0;
	}

	bool ClientPlan::supports_section_control() const
	{
		return std::any_of(booms.cbegin(), booms.cend(), [](const BoomPlan &boom) {
			return !boom.sections.empty() && !boom.setpointCondensedDdis.empty();
		});
	}

	std::size_t ClientPlan::section_count() const
	{
		std::size_t count = 0;
		for (const auto &boom : booms) count += boom.sections.size();
		return count;
	}

	ClientPlan build_client_plan(isobus::DeviceDescriptorObjectPool &pool)
	{
		ClientPlan plan;

		// Every element with the process data it references. A process data object may be
		// referenced by several elements, and then belongs to each of them.
		std::map<std::uint16_t, ElementInfo> elements; // by object ID
		for (std::uint16_t i = 0; i < pool.size(); ++i)
		{
			auto element = std::dynamic_pointer_cast<DeviceElementObject>(pool.get_object_by_index(i));
			if (nullptr == element) continue;
			ElementInfo info;
			info.number = element->get_element_number();
			info.parentObjectId = element->get_parent_object();
			info.type = element->get_type();
			for (const auto childId : child_object_ids(*element))
			{
				auto processData = std::dynamic_pointer_cast<DeviceProcessDataObject>(pool.get_object_by_id(childId));
				if (nullptr != processData) info.processData.push_back(processData);
			}
			elements.emplace(element->get_object_id(), std::move(info));
		}

		// Booms are the elements that carry condensed work states for their sections.
		std::map<std::uint16_t, BoomPlan> boomsByObject;
		for (const auto &[objectId, info] : elements)
		{
			BoomPlan boom;
			boom.element = info.number;
			for (const auto &processData : info.processData)
			{
				const auto ddi = processData->get_ddi();
				if (is_setpoint_condensed_work_state(ddi) && processData->has_property(DeviceProcessDataObject::PropertiesBit::Settable))
				{
					boom.setpointCondensedDdis.push_back(ddi);
				}
				else if (is_actual_condensed_work_state(ddi))
				{
					boom.actualCondensedDdis.push_back(ddi);
				}
			}
			if (!boom.setpointCondensedDdis.empty() || !boom.actualCondensedDdis.empty())
			{
				std::sort(boom.setpointCondensedDdis.begin(), boom.setpointCondensedDdis.end());
				std::sort(boom.actualCondensedDdis.begin(), boom.actualCondensedDdis.end());
				boomsByObject.emplace(objectId, std::move(boom));
			}
		}

		// A section belongs to the nearest boom above it (sections may sit on sub-booms).
		for (const auto &[objectId, info] : elements)
		{
			if (DeviceElementObject::Type::Section != info.type) continue;
			std::uint16_t parent = info.parentObjectId;
			for (int depth = 0; depth < 16; ++depth)
			{
				auto boom = boomsByObject.find(parent);
				if (boom != boomsByObject.end())
				{
					boom->second.sections.push_back(info.number);
					break;
				}
				auto element = elements.find(parent);
				if ((element == elements.end()) || (element->second.parentObjectId == parent)) break;
				parent = element->second.parentObjectId;
			}
		}
		for (auto &[objectId, boom] : boomsByObject)
		{
			if (boom.sections.empty()) continue;
			// Condensed work states number the sections of a boom in element number order.
			std::sort(boom.sections.begin(), boom.sections.end());
			plan.booms.push_back(std::move(boom));
		}
		std::sort(plan.booms.begin(), plan.booms.end(), [](const BoomPlan &left, const BoomPlan &right) { return left.element < right.element; });

		for (const auto &[objectId, info] : elements)
		{
			for (const auto &processData : info.processData)
			{
				const auto ddi = processData->get_ddi();
				const bool settable = processData->has_property(DeviceProcessDataObject::PropertiesBit::Settable);

				if ((static_cast<std::uint16_t>(DDI::SectionControlState) == ddi) && settable && !plan.sectionControlStateElement)
				{
					plan.sectionControlStateElement = info.number;
				}
				if (settable && is_rate_setpoint(ddi))
				{
					plan.rateSetpoints.push_back({ ddi, info.number, processData->get_designator() });
				}
				if ((static_cast<std::uint16_t>(DDI::PrescriptionControlState) == ddi) && settable)
				{
					plan.prescriptionControlStateElements.push_back(info.number);
				}

				// TC-BAS: asking for this DDI makes the client report its default data set
				// with the triggers it defines for it.
				if (static_cast<std::uint16_t>(DDI::RequestDefaultProcessData) == ddi)
				{
					plan.setupCommands.push_back({ TcCommand::Kind::RequestValue, ddi, info.number, 0 });
				}
				// Section control and coverage follow the work states, so the client reports
				// every change of them.
				const bool workState = (static_cast<std::uint16_t>(DDI::ActualWorkState) == ddi) ||
				  (static_cast<std::uint16_t>(DDI::SectionControlState) == ddi) ||
				  (static_cast<std::uint16_t>(DDI::PrescriptionControlState) == ddi) ||
				  is_actual_condensed_work_state(ddi);
				if (workState && processData->has_trigger_method(DeviceProcessDataObject::AvailableTriggerMethods::OnChange))
				{
					plan.setupCommands.push_back({ TcCommand::Kind::ChangeThreshold, ddi, info.number, 1 });
				}
			}
		}
		return plan;
	}

	std::uint32_t encode_condensed_work_state(const std::vector<bool> &states, std::size_t firstSection)
	{
		std::uint32_t value = 0;
		for (std::size_t i = 0; i < 16; ++i)
		{
			const std::size_t section = firstSection + i;
			const std::uint32_t state = (section < states.size()) ? (states[section] ? 0x1U : 0x0U) : 0x3U;
			value |= state << (2 * i);
		}
		return value;
	}

	std::vector<bool> decode_condensed_work_state(std::uint32_t value, std::size_t count)
	{
		std::vector<bool> states(std::min<std::size_t>(count, 16), false);
		for (std::size_t i = 0; i < states.size(); ++i)
		{
			states[i] = (((value >> (2 * i)) & 0x3U) == 0x1U);
		}
		return states;
	}
} // namespace agisotc
