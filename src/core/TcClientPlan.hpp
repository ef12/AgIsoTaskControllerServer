//================================================================================================
/// @file TcClientPlan.hpp
///
/// @brief What a TC server does with a client once its DDOP is active: the process data it sets
/// up for TC-BAS, and the booms, sections and rates it controls for TC-SC and rate control.
/// Derived from the DDOP alone (ISO 11783-10), without any Qt or bus dependency.
//================================================================================================
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "isobus/isobus/isobus_device_descriptor_object_pool.hpp"
#include "isobus/isobus/isobus_task_controller_client_objects.hpp"

namespace agisotc
{
	/// @brief One process data command from the TC to a client.
	struct TcCommand
	{
		enum class Kind
		{
			RequestValue,
			SetValue,
			TimeInterval,
			ChangeThreshold
		};

		Kind kind = Kind::RequestValue;
		std::uint16_t ddi = 0;
		std::uint16_t element = 0;
		std::int32_t value = 0;

		bool operator==(const TcCommand &other) const
		{
			return (kind == other.kind) && (ddi == other.ddi) && (element == other.element) && (value == other.value);
		}
	};

	/// @brief A boom: the device element whose condensed work states cover its sections.
	struct BoomPlan
	{
		std::uint16_t element = 0; ///< Element number that carries the condensed work states.
		std::vector<std::uint16_t> sections; ///< Section element numbers, in condensed work state order.
		std::vector<std::uint16_t> setpointCondensedDdis; ///< Settable Setpoint Condensed Work State DDIs present (290..305).
		std::vector<std::uint16_t> actualCondensedDdis; ///< Actual Condensed Work State DDIs present (161..176).
	};

	/// @brief A settable application rate setpoint the TC can command (rate control).
	struct RateSetpoint
	{
		std::uint16_t ddi = 0;
		std::uint16_t element = 0;
		std::string name; ///< DDOP designator of the process data, may be empty.
	};

	/// @brief Everything the TC derives from one client's DDOP.
	struct ClientPlan
	{
		std::vector<BoomPlan> booms;
		/// Element with a settable Section Control State (DDI 160), which switches the client
		/// between manual and automatic (TC) section control.
		std::optional<std::uint16_t> sectionControlStateElement;
		std::vector<RateSetpoint> rateSetpoints;
		/// Elements with a settable Prescription Control State (DDI 158). A client may accept
		/// rate setpoints only while this is automatic (1).
		std::vector<std::uint16_t> prescriptionControlStateElements;
		/// Commands to send once the pool is active: the TC-BAS default data request and
		/// on-change triggers for the work states section control and coverage depend on.
		std::vector<TcCommand> setupCommands;

		/// @brief True when the client accepts section setpoints on at least one boom.
		bool supports_section_control() const;
		/// @brief Number of sections over all booms.
		std::size_t section_count() const;
	};

	/// @brief The object IDs an element references, read one by one.
	/// @note DeviceElementObject::get_child_object_ids() is not used: its DataSpan scales the
	/// already-typed pointer by sizeof(uint16_t) again, so it runs past the end of the list.
	std::vector<std::uint16_t> child_object_ids(isobus::task_controller_object::DeviceElementObject &element);

	/// @brief Parses a client's binary DDOP. The layout depends on the TC version the client
	/// reported (version 4 adds an extended structure label to the device object), so that
	/// version is tried first, then the others.
	/// @param[in] binary The pool, all of its transfers together.
	/// @param[in] clientVersion Version from the client's technical capabilities, 0 if unknown.
	/// @param[out] pool The parsed pool, cleared when no version fits.
	/// @returns The version that parsed, or 0 if none did.
	std::uint8_t parse_client_pool(const std::vector<std::uint8_t> &binary, std::uint8_t clientVersion, isobus::DeviceDescriptorObjectPool &pool);

	/// @brief Derives the plan from an active, parsed DDOP.
	ClientPlan build_client_plan(isobus::DeviceDescriptorObjectPool &pool);

	/// @brief Encodes up to 16 section states as one condensed work state value (2 bits each,
	/// 00 off, 01 on, 11 not installed / don't care for sections beyond the list).
	/// @param[in] states Section states of the whole boom, in condensed order.
	/// @param[in] firstSection Index of the first section of this group of 16.
	std::uint32_t encode_condensed_work_state(const std::vector<bool> &states, std::size_t firstSection);

	/// @brief Decodes a condensed work state into on/off per section (01 is on, anything else off).
	std::vector<bool> decode_condensed_work_state(std::uint32_t value, std::size_t count);

	/// @brief True for the DDIs of an Actual Condensed Work State (161..176).
	bool is_actual_condensed_work_state(std::uint16_t ddi);
	/// @brief True for the DDIs of a Setpoint Condensed Work State (290..305).
	bool is_setpoint_condensed_work_state(std::uint16_t ddi);
} // namespace agisotc
