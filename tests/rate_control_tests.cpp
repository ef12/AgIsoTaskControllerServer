// Tests for TC-GEO: the prescription map, the ISOXML import, the rate plan derived from a DDOP
// (control channels and multi-rate sub-booms) and the rate controller.
#include "IsoXmlTaskData.hpp"
#include "PrescriptionMap.hpp"
#include "RatePlan.hpp"

#include "isobus/isobus/isobus_device_descriptor_object_pool.hpp"
#include "isobus/isobus/isobus_standard_data_description_indices.hpp"
#include "isobus/isobus/isobus_task_controller_client_objects.hpp"

#include <algorithm>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
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

	using agisotc::GeoPoint;
	using agisotc::PrescriptionSource;
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

	bool contains(const std::vector<TcCommand> &commands, TcCommand wanted)
	{
		return std::find(commands.cbegin(), commands.cend(), wanted) != commands.cend();
	}

	void add_children(isobus::DeviceDescriptorObjectPool &pool, std::uint16_t elementId, std::vector<std::uint16_t> children)
	{
		auto element = std::dynamic_pointer_cast<isobus::task_controller_object::DeviceElementObject>(pool.get_object_by_id(elementId));
		for (const auto child : children) element->add_reference_to_child_object(child);
	}

	isobus::DeviceDescriptorObjectPool round_trip(isobus::DeviceDescriptorObjectPool &builder)
	{
		std::vector<std::uint8_t> binary;
		CHECK(builder.generate_binary_object_pool(binary));
		isobus::DeviceDescriptorObjectPool pool;
		CHECK(4 == agisotc::parse_client_pool(binary, 4, pool));
		return pool;
	}

	/// A planter with two operations (ISO 11783-10 F.22, F.24): a seeding boom (element 1, a
	/// function) with a Prescription Control State and a seed rate, and two sub-booms (elements 2
	/// and 3) with a seed rate each, the sections below them; and a fertilizer function (element
	/// 10) with its own state and a mass rate, and a bin (element 11) under it. The seed rate
	/// process data objects are shared by the boom and its sub-booms, as a DDOP may do.
	isobus::DeviceDescriptorObjectPool planter_pool()
	{
		isobus::DeviceDescriptorObjectPool builder;
		builder.add_device("Planter", "1.0", "1", "PLANTER", { 1, 2, 3, 4, 5, 6, 7 }, {}, 0);
		builder.add_device_element("Device", 0, 0, Type::Device, 1);
		builder.add_device_element("Seeding", 1, 1, Type::Function, 2);
		builder.add_device_element("Left half", 2, 2, Type::Function, 3);
		builder.add_device_element("Right half", 3, 2, Type::Function, 4);
		builder.add_device_element("Section 1", 20, 3, Type::Section, 5);
		builder.add_device_element("Section 2", 21, 4, Type::Section, 6);
		builder.add_device_element("Fertilizer", 10, 1, Type::Function, 7);
		builder.add_device_element("Hopper", 11, 7, Type::Bin, 8);

		builder.add_device_process_data("Seed state", ddi(DDI::PrescriptionControlState), NO_PRESENTATION, SETTABLE, ON_CHANGE, 100);
		builder.add_device_process_data("Seed rate", ddi(DDI::SetpointCountPerAreaApplicationRate), NO_PRESENTATION, SETTABLE, 0, 101);
		builder.add_device_process_data("Actual seed rate", ddi(DDI::ActualCountPerAreaApplicationRate), NO_PRESENTATION, 0, TIME_INTERVAL | ON_CHANGE, 102);
		builder.add_device_process_data("Practice", ddi(DDI::ActualCulturalPractice), NO_PRESENTATION, 0, 0, 103);
		builder.add_device_process_data("Seed spacing", ddi(DDI::SetpointSpacingApplicationRate), NO_PRESENTATION, SETTABLE, 0, 104);
		builder.add_device_process_data("Section control", ddi(DDI::SectionControlState), NO_PRESENTATION, SETTABLE, ON_CHANGE, 105);
		builder.add_device_property("Latency", 300, ddi(DDI::PhysicalSetpointTimeLatency), NO_PRESENTATION, 106);
		builder.add_device_process_data("Fert state", ddi(DDI::PrescriptionControlState), NO_PRESENTATION, SETTABLE, ON_CHANGE, 110);
		builder.add_device_process_data("Fert rate", ddi(DDI::SetpointMassPerAreaApplicationRate), NO_PRESENTATION, SETTABLE, 0, 111);
		builder.add_device_property("Instance", 0, ddi(DDI::ElementTypeInstance), NO_PRESENTATION, 112);
		builder.add_device_property("Practice", 1, ddi(DDI::ActualCulturalPractice), NO_PRESENTATION, 113);

		add_children(builder, 2, { 100, 101, 102, 103, 104, 105, 106 });
		add_children(builder, 3, { 101, 102, 104 });
		add_children(builder, 4, { 101, 102, 104 });
		add_children(builder, 7, { 110, 111, 113 });
		add_children(builder, 8, { 112 });
		return round_trip(builder);
	}

	void test_rate_plan()
	{
		auto pool = planter_pool();
		const auto plan = agisotc::build_rate_plan(pool);

		CHECK(plan.channels.size() == 2);
		CHECK(plan.control_channel_count() == 2);
		if (plan.channels.size() != 2) return;
		CHECK((plan.channels[0].element == 1) && plan.channels[0].hasPrescriptionControlState);
		CHECK((plan.channels[1].element == 10) && plan.channels[1].hasPrescriptionControlState);
		CHECK(plan.channels[0].groups.size() == 2); // count per area and spacing
		CHECK(plan.channels[1].groups.size() == 1);

		const auto &seed = plan.groups[plan.channels[0].groups[0]];
		CHECK(seed.ddi == ddi(DDI::SetpointCountPerAreaApplicationRate));
		CHECK(seed.top && (plan.targets[*seed.top].element == 1) && !plan.targets[*seed.top].sub);
		CHECK(seed.subs.size() == 2);
		if (seed.subs.size() == 2)
		{
			CHECK((plan.targets[seed.subs[0]].element == 2) && plan.targets[seed.subs[0]].sub);
			CHECK(plan.targets[seed.subs[1]].element == 3);
			CHECK((plan.targets[seed.subs[1]].chain == std::vector<std::uint16_t>{ 3, 1, 0 }));
		}
		// A map goes to each sub-boom at its own position, a fixed rate to the boom, which
		// forwards it to its sub-booms.
		CHECK(seed.map_targets() == seed.subs);
		CHECK((seed.fixed_targets() == std::vector<std::size_t>{ *seed.top }));

		const auto &fertilizer = plan.groups[plan.channels[1].groups[0]];
		CHECK(fertilizer.ddi == ddi(DDI::SetpointMassPerAreaApplicationRate));
		CHECK(fertilizer.top && fertilizer.subs.empty());
		CHECK(!fertilizer.bin); // the rate sits on the function, not in the bin

		// Latency and practice are inherited along the chain: properties at once, process data
		// once received.
		const auto &left = plan.targets[seed.subs.front()];
		CHECK(agisotc::inherited_value(plan, left.chain, ddi(DDI::PhysicalSetpointTimeLatency), nullptr) == 300);
		CHECK(!agisotc::inherited_value(plan, left.chain, ddi(DDI::ActualCulturalPractice), nullptr));
		std::map<std::pair<std::uint16_t, std::uint16_t>, std::int32_t> received = { { { 1, ddi(DDI::ActualCulturalPractice) }, 2 } };
		auto lookup = [&received](std::uint16_t element, std::uint16_t value) -> std::optional<std::int32_t> {
			const auto found = received.find({ element, value });
			return (found == received.end()) ? std::nullopt : std::optional<std::int32_t>(found->second);
		};
		CHECK(agisotc::inherited_value(plan, left.chain, ddi(DDI::ActualCulturalPractice), lookup) == 2);
		CHECK(agisotc::inherited_value(plan, plan.targets[*fertilizer.top].chain, ddi(DDI::ActualCulturalPractice), lookup) == 1);

		// Set-up: the practice held as process data is asked for, and the actual rates are
		// measured at a time interval on the boom and each sub-boom.
		CHECK(contains(plan.setupCommands, { TcCommand::Kind::RequestValue, ddi(DDI::ActualCulturalPractice), 1, 0 }));
		for (const std::uint16_t element : { 1, 2, 3 })
		{
			CHECK(contains(plan.setupCommands, { TcCommand::Kind::TimeInterval, ddi(DDI::ActualCountPerAreaApplicationRate), element, 1000 }));
		}
		CHECK(plan.group_of(seed.subs.back()) == plan.channels[0].groups[0]);

		// Another settable DDI becomes a setpoint when a map holds it; control DDIs never do.
		const auto extra = agisotc::build_rate_plan(pool, { ddi(DDI::SectionControlState) });
		CHECK(extra.targets.size() == plan.targets.size());
	}

	void test_rate_plan_without_state()
	{
		// An older client: a rate in a bin, no Prescription Control State anywhere.
		isobus::DeviceDescriptorObjectPool builder;
		builder.add_device("Sprayer", "1.0", "1", "SPRAYER", { 1, 2, 3, 4, 5, 6, 7 }, {}, 0);
		builder.add_device_element("Device", 0, 0, Type::Device, 1);
		builder.add_device_element("Tank", 30, 1, Type::Bin, 2);
		builder.add_device_process_data("Rate", ddi(DDI::SetpointVolumePerAreaApplicationRate), NO_PRESENTATION, SETTABLE, 0, 100);
		builder.add_device_process_data("Not settable", ddi(DDI::SetpointMassPerAreaApplicationRate), NO_PRESENTATION, 0, 0, 101);
		add_children(builder, 2, { 100, 101 });
		auto pool = round_trip(builder);
		const auto plan = agisotc::build_rate_plan(pool);
		CHECK(plan.channels.size() == 1);
		CHECK(plan.control_channel_count() == 0);
		CHECK(plan.targets.size() == 1);
		if (plan.groups.size() == 1)
		{
			CHECK((plan.channels[0].element == 30) && !plan.channels[0].hasPrescriptionControlState);
			CHECK(plan.groups[0].bin && (*plan.groups[0].bin == 30));
			CHECK(plan.groups[0].top && plan.groups[0].subs.empty());
		}
		CHECK(agisotc::actual_rate_ddi(ddi(DDI::SetpointPercentageApplicationRate)) == ddi(DDI::ActualPercentageApplicationRate));
		CHECK(agisotc::actual_rate_ddi(ddi(DDI::SetpointSpacingApplicationRate)) == ddi(DDI::ActualSpacingApplicationRate));
		CHECK(agisotc::actual_rate_ddi(ddi(DDI::PrescriptionControlState)) == 0);
	}

	void test_rate_controller()
	{
		auto pool = planter_pool();
		const auto plan = agisotc::build_rate_plan(pool);
		agisotc::RateController controller;
		controller.reset(plan);
		const auto state = ddi(DDI::PrescriptionControlState);
		const auto rate = ddi(DDI::SetpointCountPerAreaApplicationRate);
		const auto &seed = plan.groups[plan.channels[0].groups[0]];
		std::vector<std::optional<std::int32_t>> wanted(plan.targets.size());
		wanted[seed.subs[0]] = 70000;
		wanted[seed.subs[1]] = 90000;

		CHECK(controller.update(wanted, 0).empty()); // nothing before the channel is engaged
		const auto engage = controller.set_engaged({ true, false }, 0);
		CHECK((engage == std::vector<TcCommand>{ { TcCommand::Kind::SetValue, state, 1, 1 } }));
		const auto first = controller.update(wanted, 10);
		CHECK((first == std::vector<TcCommand>{ { TcCommand::Kind::SetValue, rate, 2, 70000 }, { TcCommand::Kind::SetValue, rate, 3, 90000 } }));
		CHECK(controller.update(wanted, 500).empty()); // unchanged
		wanted[seed.subs[0]] = 75000;
		CHECK(controller.update(wanted, 100).empty()); // changed, but too soon after the last one
		const auto changed = controller.update(wanted, 300);
		CHECK((changed == std::vector<TcCommand>{ { TcCommand::Kind::SetValue, rate, 2, 75000 } }));
		CHECK(controller.update(wanted, 2020).size() == 1); // heartbeat of the right half
		CHECK(controller.commanded()[seed.subs[0]] == 75000);

		// The state again, then every setpoint again after it (the client resets them).
		const auto again = controller.reassert(2100);
		CHECK((again == std::vector<TcCommand>{ { TcCommand::Kind::SetValue, state, 1, 1 } }));
		CHECK(controller.update(wanted, 2110).size() == 2);

		const auto release = controller.set_engaged({ false, false }, 3000);
		CHECK((release == std::vector<TcCommand>{ { TcCommand::Kind::SetValue, state, 1, 0 } }));
		CHECK(!controller.any_engaged());
		CHECK(controller.update(wanted, 6000).empty());
	}

	GeoPoint at(double northM, double eastM)
	{
		// Points around 52 N 5 E, metres north and east of it.
		return { 52.0 + (northM / agisotc::metres_per_degree_latitude()), 5.0 + (eastM / agisotc::metres_per_degree_longitude(52.0)) };
	}

	std::vector<GeoPoint> square(double south, double west, double size)
	{
		return { at(south, west), at(south, west + size), at(south + size, west + size), at(south + size, west), at(south, west) };
	}

	void test_prescription_zones()
	{
		agisotc::Prescription map;
		agisotc::TreatmentZone low;
		low.code = 1;
		low.values.push_back({ ddi(DDI::SetpointMassPerAreaApplicationRate), 10000 });
		agisotc::PrescriptionPolygon lowArea;
		lowArea.exterior = square(0, 0, 100);
		lowArea.holes.push_back(square(40, 40, 20));
		low.polygons.push_back(lowArea);
		agisotc::TreatmentZone fallback;
		fallback.code = 2;
		fallback.values.push_back({ ddi(DDI::SetpointMassPerAreaApplicationRate), 15000 });
		agisotc::TreatmentZone outside;
		outside.code = 3;
		outside.values.push_back({ ddi(DDI::SetpointMassPerAreaApplicationRate), 0 });
		map.zones = { low, fallback, outside };
		map.defaultZone = 2;
		map.outOfFieldZone = 3;
		map.positionLostZone = 2;
		map.update_layers();

		CHECK(map.layers().size() == 1);
		CHECK((map.layers()[0].minimum == 10000) && (map.layers()[0].maximum == 15000)); // out-of-field 0 aside
		auto lookup = map.value_at(0, at(10, 10), true);
		CHECK((lookup.value == 10000) && (lookup.source == PrescriptionSource::Zone) && (lookup.zoneCode == 1));
		lookup = map.value_at(0, at(50, 50), true); // in the hole
		CHECK((lookup.value == 15000) && (lookup.source == PrescriptionSource::Default));
		lookup = map.value_at(0, at(10, 10), false);
		CHECK((lookup.value == 0) && (lookup.source == PrescriptionSource::OutOfField));
		lookup = map.value_without_position(0);
		CHECK((lookup.value == 15000) && (lookup.source == PrescriptionSource::PositionLost));
		CHECK(!map.value_at(1, at(10, 10), true).value); // no such layer

		GeoPoint southWest;
		GeoPoint northEast;
		CHECK(map.bounds(southWest, northEast));
		CHECK((southWest.latitude < at(1, 0).latitude) && (northEast.longitude > at(0, 99).longitude));

		// A zone drawn later lies in front of the earlier ones.
		agisotc::PrescriptionValue high{ ddi(DDI::SetpointMassPerAreaApplicationRate), 20000 };
		const auto code = agisotc::add_polygon_zone(map, square(5, 5, 10), high, "High");
		CHECK(code == 4);
		CHECK(map.value_at(0, at(10, 10), true).value == 20000);
		CHECK(map.value_at(0, at(30, 30), true).value == 10000);
	}

	void test_prescription_grids()
	{
		// Grid type 1: zone codes, 2 rows of 3 cells of 10 m, starting in the south-west.
		agisotc::Prescription coded;
		for (std::uint8_t code = 1; code <= 3; ++code)
		{
			agisotc::TreatmentZone zone;
			zone.code = code;
			zone.values.push_back({ ddi(DDI::SetpointVolumePerAreaApplicationRate), code * 1000 });
			coded.zones.push_back(zone);
		}
		agisotc::PrescriptionGrid grid;
		grid.minimumNorth = at(0, 0).latitude;
		grid.minimumEast = at(0, 0).longitude;
		grid.cellNorthSize = 10.0 / agisotc::metres_per_degree_latitude();
		grid.cellEastSize = 10.0 / agisotc::metres_per_degree_longitude(52.0);
		grid.columns = 3;
		grid.rows = 2;
		grid.type = 1;
		grid.zoneCodes = { 1, 2, 3, 3, 2, 1 };
		coded.grid = grid;
		coded.update_layers();
		CHECK(coded.value_at(0, at(5, 5), true).value == 1000);
		CHECK(coded.value_at(0, at(5, 25), true).value == 3000);
		CHECK(coded.value_at(0, at(15, 5), true).value == 3000); // second row runs east again
		CHECK(coded.value_at(0, at(15, 25), true).source == PrescriptionSource::Grid);
		CHECK(!coded.value_at(0, at(25, 5), true).value); // north of the grid
		CHECK(!coded.value_at(0, at(5, -1), true).value); // west of it

		// Grid type 2: values per cell, generated, two layers in one record.
		agisotc::Prescription generated;
		const agisotc::PrescriptionValue seedRate{ ddi(DDI::SetpointCountPerAreaApplicationRate), 0 };
		agisotc::add_test_layer(generated, at(0, 0), at(40, 40), 10.0, seedRate, agisotc::TestPattern::Stripes, 60000, 90000, 20.0);
		CHECK(generated.grid && (generated.grid->type == 2) && (generated.grid->columns == 4) && (generated.grid->rows == 4));
		CHECK(generated.value_at(0, at(35, 5), true).value == 60000);
		CHECK(generated.value_at(0, at(5, 25), true).value == 90000);
		agisotc::PrescriptionValue fertilizer{ ddi(DDI::SetpointMassPerAreaApplicationRate), 0 };
		fertilizer.culturalPractice = 1;
		agisotc::add_test_layer(generated, at(0, 0), at(40, 40), 10.0, fertilizer, agisotc::TestPattern::Bands, 100, 200, 10.0);
		CHECK(generated.layers().size() == 2);
		CHECK(generated.grid->values.size() == 32);
		CHECK(generated.value_at(0, at(5, 25), true).value == 90000); // the first layer kept
		CHECK(generated.value_at(1, at(5, 25), true).value == 100);
		CHECK(generated.value_at(1, at(15, 25), true).value == 200);
		CHECK((generated.layers()[0].minimum == 60000) && (generated.layers()[0].maximum == 90000));
		agisotc::add_test_layer(generated, at(0, 0), at(40, 40), 10.0, seedRate, agisotc::TestPattern::Gradient, 0, 1000, 10.0);
		CHECK(generated.layers().size() == 2); // replaced, not added
		CHECK(generated.value_at(0, at(5, 5), true).value == 125);
		CHECK(generated.value_at(0, at(5, 35), true).value == 875);
		CHECK(generated.value_at(1, at(15, 25), true).value == 200);
		CHECK(!generated.describe().empty());

		// Layers are told apart by practice and instance (ISO 11783-10 D.41).
		const auto &layers = generated.layers();
		CHECK(agisotc::match_layer(layers, ddi(DDI::SetpointMassPerAreaApplicationRate), 1, std::nullopt) == 1);
		CHECK(agisotc::match_layer(layers, ddi(DDI::SetpointMassPerAreaApplicationRate), std::nullopt, std::nullopt) == 1);
		CHECK(!agisotc::match_layer(layers, ddi(DDI::SetpointMassPerAreaApplicationRate), 2, std::nullopt));
		CHECK(!agisotc::match_layer(layers, ddi(DDI::SetpointVolumePerAreaApplicationRate), std::nullopt, std::nullopt));
		std::vector<agisotc::PrescriptionLayer> two(2);
		two[0].ddi = two[1].ddi = 6;
		two[1].elementTypeInstance = 1;
		CHECK(agisotc::match_layer(two, 6, std::nullopt, 1) == 1); // the closer match wins
		CHECK(agisotc::match_layer(two, 6, std::nullopt, 0) == 0);
	}

	void test_xml_parser()
	{
		agisotc::XmlElement root;
		std::string error;
		CHECK(agisotc::parse_xml("\xEF\xBB\xBF<?xml version=\"1.0\"?><!-- c --><A x='1' y=\"a&amp;b&#65;\"><B/><C></C></A>", root, error));
		CHECK((root.name == "A") && (root.children.size() == 2) && (root.children[1].name == "C"));
		CHECK((root.attribute("y") != nullptr) && (*root.attribute("y") == "a&bA"));
		CHECK(!agisotc::parse_xml("<A><B></A>", root, error));
		CHECK(!agisotc::parse_xml("<A x=1/>", root, error));
		CHECK(!agisotc::parse_xml("<A/><B/>", root, error));
	}

	void test_task_data_import()
	{
		const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<ISO11783_TaskData VersionMajor="4" VersionMinor="2" DataTransferOrigin="1">
  <VPN A="VPN1" B="0" C="0.01" D="0" E="kg/ha"/>
  <PFD A="PFD1" C="North field">
    <PLN A="1">
      <LSG A="1">
        <PNT A="2" C="52.0" D="5.0"/><PNT A="2" C="52.0" D="5.01"/><PNT A="2" C="52.01" D="5.01"/><PNT A="2" C="52.01" D="5.0"/>
      </LSG>
    </PLN>
  </PFD>
  <XFR A="TSK00002" B="1"/>
  <TSK A="TSK1" B="Fertilize" E="PFD1" G="1" H="9">
    <TZN A="1" B="Low">
      <PDV A="0006" B="5000" E="VPN1" F="1"/>
      <PLN A="2"><LSG A="1"><PNT A="2" C="52.0" D="5.0"/><PNT A="2" C="52.0" D="5.005"/><PNT A="2" C="52.005" D="5.005"/><PNT A="2" C="52.005" D="5.0"/></LSG></PLN>
    </TZN>
    <TZN A="9" B="Default"><PDV A="0006" B="7500" E="VPN1" F="1"/></TZN>
  </TSK>
</ISO11783_TaskData>)";
		const std::string external = R"(<XFC><TSK A="TSK2" B="Seed" G="1">
  <TZN A="1" B="Values"><PDV A="000B" B="0"/><PDV A="0006" B="0"/></TZN>
  <GRD A="52.0" B="5.0" C="0.001" D="0.001" E="2" F="1" G="GRD00001" I="2" J="1"/>
</TSK></XFC>)";
		const std::vector<std::int32_t> cells = { 80000, 100, 90000, 200 }; // two cells of two values
		std::vector<std::uint8_t> gridFile;
		for (const auto value : cells)
		{
			const auto raw = static_cast<std::uint32_t>(value);
			for (int shift = 0; shift < 32; shift += 8) gridFile.push_back(static_cast<std::uint8_t>(raw >> shift));
		}
		auto reader = [&](const std::string &name) -> std::optional<std::vector<std::uint8_t>> {
			if (name == "GRD00001.bin") return gridFile;
			if (name == "TSK00002.XML") return std::vector<std::uint8_t>(external.begin(), external.end());
			return std::nullopt;
		};
		agisotc::TaskDataImport imported;
		std::string error;
		CHECK(agisotc::import_task_data(xml, reader, imported, error));
		CHECK(imported.fields.size() == 1);
		CHECK(imported.tasks.size() == 2);
		if ((imported.fields.size() != 1) || (imported.tasks.size() != 2)) return;
		CHECK((imported.fields[0].name == "North field") && (imported.fields[0].boundary.size() == 4));

		const auto &fertilize = imported.tasks[0];
		CHECK((fertilize.name == "Fertilize") && (fertilize.partfieldId == "PFD1"));
		const auto &layers = fertilize.prescription.layers();
		CHECK((layers.size() == 1) && (layers[0].ddi == 6) && (layers[0].culturalPractice == 1));
		CHECK(layers[0].presentation && (layers[0].presentation->unit == "kg/ha") && (layers[0].presentation->scale == 0.01));
		CHECK(fertilize.prescription.value_at(0, { 52.001, 5.001 }, true).value == 5000);
		CHECK(fertilize.prescription.value_at(0, { 52.008, 5.008 }, true).value == 7500); // default zone

		const auto &seed = imported.tasks[1];
		CHECK(seed.prescription.grid && (seed.prescription.grid->type == 2));
		CHECK(seed.prescription.layers().size() == 2);
		CHECK(seed.prescription.value_at(0, { 52.0005, 5.0015 }, true).value == 90000);
		CHECK(seed.prescription.value_at(1, { 52.0005, 5.0005 }, true).value == 100);
		CHECK(imported.warnings.empty());

		// A grid file that is missing is a warning, not an error.
		agisotc::TaskDataImport partial;
		CHECK(agisotc::import_task_data(xml, [&](const std::string &name) -> std::optional<std::vector<std::uint8_t>> {
			if (name == "TSK00002.XML") return std::vector<std::uint8_t>(external.begin(), external.end());
			return std::nullopt;
		}, partial, error));
		CHECK(!partial.warnings.empty());
		CHECK(!agisotc::import_task_data("<Other/>", reader, partial, error));
	}
} // namespace

int main()
{
	test_rate_plan();
	test_rate_plan_without_state();
	test_rate_controller();
	test_prescription_zones();
	test_prescription_grids();
	test_xml_parser();
	test_task_data_import();
	if (0 == failures) std::printf("rate_control_tests: all passed\n");
	return (0 == failures) ? 0 : 1;
}
