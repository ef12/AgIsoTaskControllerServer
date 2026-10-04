// Tests for the TC plan derived from a DDOP, the section controller and the coverage map.
#include "CoverageMap.hpp"
#include "SectionController.hpp"
#include "SectionPlanner.hpp"
#include "TcClientPlan.hpp"

#include "isobus/isobus/isobus_standard_data_description_indices.hpp"
#include "isobus/isobus/isobus_task_controller_client_objects.hpp"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <vector>

namespace
{
	int failures = 0;

#define CHECK(condition)                                                               \
	do                                                                                 \
	{                                                                                  \
		if (!(condition))                                                              \
		{                                                                              \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);           \
			++failures;                                                                \
		}                                                                              \
	} while (false)

	using agisotc::TcCommand;
	using DDI = isobus::DataDescriptionIndex;
	using Type = isobus::task_controller_object::DeviceElementObject::Type;

	constexpr std::uint8_t SETTABLE = 0x02;
	constexpr std::uint8_t TIME_INTERVAL = 0x01;
	constexpr std::uint8_t ON_CHANGE = 0x08;
	constexpr std::uint16_t NO_PRESENTATION = 0xFFFF;

	std::uint16_t ddi(DDI value)
	{
		return static_cast<std::uint16_t>(value);
	}

	void add_children(isobus::DeviceDescriptorObjectPool &pool, std::uint16_t elementId, std::vector<std::uint16_t> children)
	{
		auto element = std::dynamic_pointer_cast<isobus::task_controller_object::DeviceElementObject>(pool.get_object_by_id(elementId));
		for (const auto child : children) element->add_reference_to_child_object(child);
	}

	/// A generic implement with two booms: boom 1 (element 1) with three sections numbered out of
	/// order, boom 2 (element 2) with two sections, and a bin (element 30) with a rate setpoint.
	/// The condensed work state objects are shared by both booms, as a DDOP may do.
	isobus::DeviceDescriptorObjectPool two_boom_pool()
	{
		isobus::DeviceDescriptorObjectPool builder;
		builder.add_device("Test implement", "1.0", "1", "TESTLBL", { 1, 2, 3, 4, 5, 6, 7 }, {}, 0);
		builder.add_device_element("Device", 0, 0, Type::Device, 1);
		builder.add_device_process_data("Default data", ddi(DDI::RequestDefaultProcessData), NO_PRESENTATION, 0, 0, 100);
		builder.add_device_process_data("SC state", ddi(DDI::SectionControlState), NO_PRESENTATION, SETTABLE, ON_CHANGE, 101);
		builder.add_device_process_data("Actual 1-16", ddi(DDI::ActualCondensedWorkState1_16), NO_PRESENTATION, 0, ON_CHANGE | TIME_INTERVAL, 102);
		builder.add_device_process_data("Setpoint 1-16", ddi(DDI::SetpointCondensedWorkState1_16), NO_PRESENTATION, SETTABLE, 0, 103);
		builder.add_device_process_data("Work state", ddi(DDI::ActualWorkState), NO_PRESENTATION, 0, TIME_INTERVAL, 104);
		builder.add_device_process_data("Rate", ddi(DDI::SetpointCountPerAreaApplicationRate), NO_PRESENTATION, SETTABLE, 0, 105);

		builder.add_device_element("Boom 1", 1, 1, Type::Function, 2);
		builder.add_device_element("Section C", 12, 2, Type::Section, 3);
		builder.add_device_element("Section A", 10, 2, Type::Section, 4);
		builder.add_device_element("Section B", 11, 2, Type::Section, 5);
		builder.add_device_element("Boom 2", 2, 1, Type::Function, 6);
		builder.add_device_element("Section D", 20, 6, Type::Section, 7);
		builder.add_device_element("Section E", 21, 6, Type::Section, 8);
		builder.add_device_element("Bin", 30, 1, Type::Bin, 9);

		add_children(builder, 1, { 100 });
		add_children(builder, 2, { 101, 102, 103, 104 });
		add_children(builder, 6, { 102, 103 });
		add_children(builder, 9, { 105 });

		// Round-trip through the binary form, as the pool arrives from a client.
		std::vector<std::uint8_t> binary;
		CHECK(builder.generate_binary_object_pool(binary));
		isobus::DeviceDescriptorObjectPool pool;
		CHECK(4 == agisotc::parse_client_pool(binary, 4, pool));
		return pool;
	}

	void test_parse_by_client_version()
	{
		// A version 3 client's device object has no extended structure label: parsed with the
		// version 4 layout it fails, so its reported version has to be used.
		isobus::DeviceDescriptorObjectPool builder(3);
		builder.add_device("V3 implement", "1.0", "1", "V3LABEL", { 1, 2, 3, 4, 5, 6, 7 }, {}, 0);
		builder.add_device_element("Device", 0, 0, Type::Device, 1);
		std::vector<std::uint8_t> binary;
		CHECK(builder.generate_binary_object_pool(binary));
		isobus::DeviceDescriptorObjectPool pool;
		CHECK(3 == agisotc::parse_client_pool(binary, 3, pool));
		CHECK(pool.size() == 2);
		CHECK(3 == agisotc::parse_client_pool(binary, 0, pool)); // version unknown: found by trying
		CHECK(0 == agisotc::parse_client_pool({ 1, 2, 3 }, 3, pool));
		CHECK(pool.size() == 0);
	}

	bool contains(const std::vector<TcCommand> &commands, TcCommand wanted)
	{
		return std::find(commands.cbegin(), commands.cend(), wanted) != commands.cend();
	}

	void test_plan()
	{
		auto pool = two_boom_pool();
		const auto plan = agisotc::build_client_plan(pool);

		CHECK(plan.booms.size() == 2);
		if (plan.booms.size() == 2)
		{
			CHECK(plan.booms[0].element == 1);
			CHECK((plan.booms[0].sections == std::vector<std::uint16_t>{ 10, 11, 12 }));
			CHECK((plan.booms[1].sections == std::vector<std::uint16_t>{ 20, 21 }));
			CHECK((plan.booms[1].setpointCondensedDdis == std::vector<std::uint16_t>{ ddi(DDI::SetpointCondensedWorkState1_16) }));
		}
		CHECK(plan.supports_section_control());
		CHECK(plan.section_count() == 5);
		CHECK(plan.sectionControlStateElement && (*plan.sectionControlStateElement == 1));

		// TC-BAS default data, and on-change reports for the work states that have that trigger.
		CHECK(contains(plan.setupCommands, { TcCommand::Kind::RequestValue, ddi(DDI::RequestDefaultProcessData), 0, 0 }));
		CHECK(contains(plan.setupCommands, { TcCommand::Kind::ChangeThreshold, ddi(DDI::SectionControlState), 1, 1 }));
		CHECK(contains(plan.setupCommands, { TcCommand::Kind::ChangeThreshold, ddi(DDI::ActualCondensedWorkState1_16), 1, 1 }));
		CHECK(contains(plan.setupCommands, { TcCommand::Kind::ChangeThreshold, ddi(DDI::ActualCondensedWorkState1_16), 2, 1 }));
		CHECK(!contains(plan.setupCommands, { TcCommand::Kind::ChangeThreshold, ddi(DDI::ActualWorkState), 1, 1 })); // time interval only
		CHECK(plan.setupCommands.size() == 4); // exactly these: nothing read past an element's child list
	}

	void test_child_object_ids()
	{
		auto pool = two_boom_pool();
		auto boom = std::dynamic_pointer_cast<isobus::task_controller_object::DeviceElementObject>(pool.get_object_by_id(2));
		CHECK(nullptr != boom);
		if (nullptr != boom)
		{
			CHECK((agisotc::child_object_ids(*boom) == std::vector<std::uint16_t>{ 101, 102, 103, 104 }));
		}
	}

	void test_condensed_encoding()
	{
		const std::vector<bool> states = { true, false, true };
		const auto value = agisotc::encode_condensed_work_state(states, 0);
		CHECK(value == 0xFFFFFFD1U); // 01, 00, 01, then 11 (not installed) for the rest
		CHECK((agisotc::decode_condensed_work_state(value, 3) == states));
		std::vector<bool> many(20, false);
		many[17] = true;
		CHECK(agisotc::encode_condensed_work_state(many, 16) == 0xFFFFFF04U); // sections 17..20
	}

	void test_section_controller()
	{
		auto pool = two_boom_pool();
		agisotc::SectionController controller;
		controller.reset(agisotc::build_client_plan(pool));
		const auto setpoint = ddi(DDI::SetpointCondensedWorkState1_16);
		const auto state = ddi(DDI::SectionControlState);

		CHECK(controller.update({ true, true, true, true, true }, 0).empty()); // not engaged yet
		auto engage = controller.set_engaged(true, 0);
		CHECK((engage == std::vector<TcCommand>{ { TcCommand::Kind::SetValue, state, 1, 1 } }));

		const std::vector<bool> wanted = { true, false, true, false, true };
		auto first = controller.update(wanted, 10);
		CHECK(first.size() == 2);
		CHECK(contains(first, { TcCommand::Kind::SetValue, setpoint, 1, static_cast<std::int32_t>(agisotc::encode_condensed_work_state({ true, false, true }, 0)) }));
		CHECK(contains(first, { TcCommand::Kind::SetValue, setpoint, 2, static_cast<std::int32_t>(agisotc::encode_condensed_work_state({ false, true }, 0)) }));
		CHECK(controller.update(wanted, 500).empty()); // unchanged, heartbeat not due
		CHECK(controller.update(wanted, 1200).size() == 2); // heartbeat

		auto boomTwoOnly = controller.update({ true, false, true, true, true }, 1300);
		CHECK((boomTwoOnly.size() == 1) && (boomTwoOnly[0].element == 2));

		auto disengage = controller.set_engaged(false, 1400);
		CHECK(disengage.size() == 3); // both booms off, then manual
		CHECK(disengage.back() == (TcCommand{ TcCommand::Kind::SetValue, state, 1, 0 }));
		CHECK(contains(disengage, { TcCommand::Kind::SetValue, setpoint, 2, static_cast<std::int32_t>(agisotc::encode_condensed_work_state({ false, false }, 0)) }));
		CHECK(controller.update(wanted, 3000).empty());
	}

	void test_section_planner()
	{
		using agisotc::SectionGround;
		// A 100 m square field: x 0..100, z -100..0 (z grows to the south).
		const std::vector<agisotc::GroundPoint> field = { { 0, 0 }, { 100, 0 }, { 100, -100 }, { 0, -100 }, { 0, 0 } };
		agisotc::CoverageMap coverage(0.25);
		auto one = [&](SectionGround section, const std::vector<agisotc::GroundPoint> &boundary = {}) {
			return agisotc::wanted_section_states({ section }, 1.0, boundary.empty() ? field : boundary, coverage, 100000)[0];
		};
		const agisotc::GroundPoint north = { 0.0, -3.0 }; // 3 m/s towards the north
		const agisotc::GroundPoint south = { 0.0, 3.0 };
		const agisotc::GroundPoint east = { 3.0, 0.0 };

		CHECK(one({ { 50, -50 }, north, 3.0 })); // in the field, moving
		CHECK(!one({ { 50, -50 }, { 0.0, -0.1 }, 3.0 })); // standing still
		CHECK(!one({ { 50, -98 }, north, 3.0 })); // leaves the field within the look-ahead
		CHECK(one({ { 50, -101 }, south, 3.0 })); // about to enter: on at the edge
		CHECK(!one({ { 50, -110 }, south, 3.0 })); // turning back, still far outside
		// Outside, travelling along the edge in a headland turn: the implement may already point
		// into the field, but the section itself does not reach it.
		CHECK(!one({ { 50, -103 }, east, 3.0 }));
		// No boundary: work everywhere.
		CHECK(agisotc::wanted_section_states({ { { 500, 500 }, north, 3.0 } }, 1.0, {}, coverage, 100000)[0]);
		// Ground covered by an earlier pass switches the section off.
		coverage.cover_swath({ 40, -60 }, { 60, -60 }, 10.0, 1000);
		CHECK(!one({ { 50, -57 }, north, 3.0 }));
		CHECK(agisotc::point_in_ring({ 50, -50 }, field));
		CHECK(!agisotc::point_in_ring({ 50, -103 }, field));
	}

	void test_coverage()
	{
		agisotc::CoverageMap coverage(0.25);
		const double first = coverage.cover_swath({ 0.0, 0.0 }, { 10.0, 0.0 }, 2.0, 5000);
		CHECK((first > 19.0) && (first < 24.0)); // a 10 m by 2 m strip, to the cell
		CHECK(coverage.is_covered({ 5.0, 0.5 }, 10000, 2000));
		CHECK(!coverage.is_covered({ 5.0, 0.5 }, 6000, 2000)); // covered only 1 s ago
		CHECK(!coverage.is_covered({ 5.0, 3.0 }, 10000, 0));
		const double overlap = coverage.cover_swath({ 0.0, 0.0 }, { 10.0, 0.0 }, 2.0, 6000);
		CHECK(overlap < 0.5); // the same strip again adds (almost) nothing
		CHECK(coverage.cover_swath({ 1.0, 1.0 }, { 1.0, 1.0 }, 2.0, 7000) == 0.0); // standing still
		coverage.clear();
		CHECK(coverage.covered_area_m2() == 0.0);
	}
} // namespace

int main()
{
	test_plan();
	test_child_object_ids();
	test_parse_by_client_version();
	test_condensed_encoding();
	test_section_controller();
	test_section_planner();
	test_coverage();
	if (0 == failures) std::printf("tc_plan_tests: all passed\n");
	return (0 == failures) ? 0 : 1;
}
