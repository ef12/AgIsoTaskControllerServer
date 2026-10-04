// TC-GEO in the bridge: variable rate (a prescription map applied by position) and multi-rate
// control (several control channels, and the sub-boom or section rates of a channel), ISO 11783-10
// 6.8.1 and F.3.4. The decisions are in the core (RatePlan, PrescriptionMap); this file feeds them
// the implement's position and the client's values, sends the commands and shows the result.
#include "TcBridge.hpp"

#include <algorithm>
#include <cmath>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>

#include "IsoXmlTaskData.hpp"
#include "SectionPlanner.hpp"

#include "isobus/isobus/isobus_device_descriptor_object_pool.hpp"
#include "isobus/isobus/isobus_standard_data_description_indices.hpp"

namespace agisotc
{
	namespace
	{
		using DDI = isobus::DataDescriptionIndex;

		/// A setpoint latency longer than this is taken as a faulty value.
		constexpr double MAX_LATENCY_S = 10.0;
		/// A target that moved further than this between two updates jumped (a new position).
		constexpr double JUMP_M = 10.0;
		constexpr std::uint64_t RATE_PUBLISH_MS = 250;
		/// The longer side of a rendered polygon map, in pixels.
		constexpr double MAP_IMAGE_PIXELS = 768.0;

		QString rate_ddi_name(std::uint16_t ddi)
		{
			switch (static_cast<DDI>(ddi))
			{
				case DDI::SetpointVolumePerAreaApplicationRate: return "Volume per area";
				case DDI::SetpointMassPerAreaApplicationRate: return "Mass per area";
				case DDI::SetpointCountPerAreaApplicationRate: return "Count per area";
				case DDI::SetpointSpacingApplicationRate: return "Spacing";
				case DDI::SetpointVolumePerVolumeApplicationRate: return "Volume per volume";
				case DDI::SetpointMassPerMassApplicationRate: return "Mass per mass";
				case DDI::SetpointVolumePerMassApplicationRate: return "Volume per mass";
				case DDI::SetpointVolumePerTimeApplicationRate: return "Volume per time";
				case DDI::SetpointMassPerTimeApplicationRate: return "Mass per time";
				case DDI::SetpointCountPerTimeApplicationRate: return "Count per time";
				case DDI::SetpointPercentageApplicationRate: return "Percentage";
				default: return QString("DDI %1").arg(ddi);
			}
		}

		QString source_text(PrescriptionSource source, std::uint8_t zone)
		{
			switch (source)
			{
				case PrescriptionSource::Zone: return QString("zone %1").arg(zone);
				case PrescriptionSource::Grid: return "grid";
				case PrescriptionSource::Default: return "default zone";
				case PrescriptionSource::OutOfField: return "out of field";
				case PrescriptionSource::PositionLost: return "position lost";
				case PrescriptionSource::None: break;
			}
			return "no value";
		}

		QString state_text(std::optional<std::int32_t> state)
		{
			if (!state) return "unknown";
			switch (*state & 0x03)
			{
				case 0: return "manual";
				case 1: return "automatic";
				case 2: return "error";
				default: return "not available";
			}
		}

		/// What makes two plans the same for the client: the channels and their setpoints.
		std::vector<std::uint32_t> plan_signature(const RatePlan &plan)
		{
			std::vector<std::uint32_t> signature;
			for (const auto &channel : plan.channels) signature.push_back(channel.element);
			for (const auto &target : plan.targets) signature.push_back((static_cast<std::uint32_t>(target.ddi) << 16) | target.element);
			return signature;
		}

		QVariant optional_value(const std::optional<std::int32_t> &value)
		{
			return value ? QVariant(static_cast<qlonglong>(*value)) : QVariant();
		}
	} // namespace

	QVariantList TcBridge::rateChannels() const
	{
		return currentRateChannels;
	}

	QVariantMap TcBridge::rateLive() const
	{
		return currentRateLive;
	}

	QString TcBridge::rateControlStatus() const
	{
		return currentRateControlStatus;
	}

	QVariantMap TcBridge::prescription() const
	{
		return currentPrescription;
	}

	VariantListModel *TcBridge::rateMarkerModel()
	{
		return &rateMarkerRows;
	}

	std::shared_ptr<PrescriptionImageSlot> TcBridge::prescriptionImageSlot() const
	{
		return prescriptionImages;
	}

	GeoPoint TcBridge::localToGeo(GroundPoint point) const
	{
		return { fieldOriginLatitude - (point.z / metres_per_degree_latitude()),
			     fieldOriginLongitude + (point.x / metres_per_degree_longitude(fieldOriginLatitude)) };
	}

	GroundPoint TcBridge::geoToLocal(GeoPoint point) const
	{
		return { (point.longitude - fieldOriginLongitude) * metres_per_degree_longitude(fieldOriginLatitude),
			     -(point.latitude - fieldOriginLatitude) * metres_per_degree_latitude() };
	}

	std::optional<std::int32_t> TcBridge::receivedValue(std::uint16_t element, std::uint16_t ddi) const
	{
		for (const auto &state : implementDdiStates)
		{
			if ((state.element == element) && (state.ddi == ddi) && state.hasValue) return state.value;
		}
		return std::nullopt;
	}

	// --- the plan -------------------------------------------------------------------------------

	void TcBridge::setupRatePlan(isobus::DeviceDescriptorObjectPool *pool, bool keepWhenSame)
	{
		const auto nowMs = steady_clock_ms();
		RatePlan plan;
		if (nullptr != pool)
		{
			std::vector<std::uint16_t> mapDdis;
			if (const auto *map = selectedPrescription())
			{
				for (const auto &layer : map->layers()) mapDdis.push_back(layer.ddi);
			}
			plan = build_rate_plan(*pool, mapDdis);
		}
		if (keepWhenSame && (plan_signature(plan) == plan_signature(rateController.plan())))
		{
			publishRateChannels(nowMs, true);
			return;
		}
		if (rateController.any_engaged())
		{
			sendTcCommands(rateController.set_engaged({}, nowMs));
		}
		rateController.reset(plan);
		const auto count = plan.targets.size();
		rateWanted.assign(count, std::nullopt);
		rateWantedSource.assign(count, PrescriptionSource::None);
		rateTargetPoints.assign(count, GroundPoint{});
		rateTargetLast.clear();
		rateTargetVelocity.assign(count, GroundPoint{});
		lastRateMotionMs = 0;

		if (!plan.channels.empty())
		{
			QStringList channels;
			for (const auto &channel : plan.channels)
			{
				QStringList rates;
				for (const auto groupIndex : channel.groups)
				{
					const auto &group = plan.groups[groupIndex];
					rates << QString("%1 (DDI %2%3)")
					           .arg(rate_ddi_name(group.ddi))
					           .arg(group.ddi)
					           .arg(group.subs.empty() ? QString() : QString(", %1 sub-rates").arg(group.subs.size()));
				}
				channels << QString("element %1 %2%3: %4")
				              .arg(channel.element)
				              .arg(QString::fromStdString(channel.name).trimmed())
				              .arg(channel.hasPrescriptionControlState ? QString() : QString(" (no prescription control state)"))
				              .arg(rates.join(", "));
			}
			logs.addLine(QString("[rate] Client %1 has %2 position-based control channel(s): %3.")
			               .arg(planClient)
			               .arg(plan.control_channel_count())
			               .arg(channels.join("; ")));
			if (static_cast<int>(plan.control_channel_count()) > supportedChannels)
			{
				logs.addLine(QString("[rate] The client has more control channels than this TC offers (%1).").arg(supportedChannels));
			}
		}
		publishRateChannels(nowMs, true);
	}

	std::optional<std::size_t> TcBridge::matchedLayer(std::size_t group) const
	{
		const auto *map = selectedPrescription();
		const auto &plan = rateController.plan();
		if ((nullptr == map) || (group >= plan.groups.size())) return std::nullopt;
		const auto &rateGroup = plan.groups[group];
		const auto targets = rateGroup.map_targets();
		if (targets.empty()) return std::nullopt;
		const auto &chain = plan.targets[targets.front()].chain;
		auto received = [this](std::uint16_t element, std::uint16_t ddi) { return receivedValue(element, ddi); };
		return match_layer(map->layers(),
		                   rateGroup.ddi,
		                   inherited_value(plan, chain, static_cast<std::uint16_t>(DDI::ActualCulturalPractice), received),
		                   inherited_value(plan, chain, static_cast<std::uint16_t>(DDI::ElementTypeInstance), received));
	}

	int TcBridge::rateGroupSource(std::size_t group) const
	{
		const auto &plan = rateController.plan();
		if (group >= plan.groups.size()) return 0;
		const auto &rateGroup = plan.groups[group];
		const auto *map = selectedPrescription();
		const auto choice = rateGroupChoices.find({ plan.channels[rateGroup.channel].element, rateGroup.ddi, rateGroup.bin ? *rateGroup.bin : -1 });
		if ((choice != rateGroupChoices.end()) && (choice->second.source >= 0))
		{
			const int source = choice->second.source;
			if ((source >= 2) && ((nullptr == map) || (static_cast<std::size_t>(source - 2) >= map->layers().size()))) return 0;
			return source;
		}
		const auto layer = matchedLayer(group);
		return layer ? (2 + static_cast<int>(*layer)) : 0;
	}

	// --- control --------------------------------------------------------------------------------

	void TcBridge::serviceRateControl(std::uint64_t nowMs)
	{
		const auto &plan = rateController.plan();
		const std::size_t count = plan.targets.size();
		if (rateWanted.size() != count) return;
		const bool connected = (planClient >= 0) && (connectedAddresses.find(static_cast<std::uint8_t>(planClient)) != connectedAddresses.end());
		const auto *map = selectedPrescription();
		auto received = [this](std::uint16_t element, std::uint16_t ddi) { return receivedValue(element, ddi); };

		// Where each target is, how it moves, and where it will be once a setpoint sent now is
		// applied (ISO 11783-10 F.3.4.6: the TC projects the position by the physical setpoint
		// time latency).
		const auto connector = connectorOffset();
		const double elapsedS = (0 == lastRateMotionMs) ? 0.0 : static_cast<double>(nowMs - lastRateMotionMs) / 1000.0;
		lastRateMotionMs = nowMs;
		const bool fresh = (rateTargetLast.size() != count);
		if (fresh) rateTargetLast.assign(count, GroundPoint{});
		for (std::size_t target = 0; target < count; ++target)
		{
			const auto &setpoint = plan.targets[target];
			const auto element = std::find_if(implementElementStates.cbegin(), implementElementStates.cend(),
			                                  [&setpoint](const ImplementElementState &state) { return state.element == setpoint.element; });
			const auto centre = implementGround((element != implementElementStates.cend()) ? elementOffset(*element) : connector, connector);
			if (fresh || (elapsedS <= 0.0) || (elapsedS > 2.0) || (std::hypot(centre.x - rateTargetLast[target].x, centre.z - rateTargetLast[target].z) > JUMP_M))
			{
				rateTargetVelocity[target] = {};
			}
			else
			{
				const GroundPoint measured = { (centre.x - rateTargetLast[target].x) / elapsedS, (centre.z - rateTargetLast[target].z) / elapsedS };
				rateTargetVelocity[target] = { (rateTargetVelocity[target].x + measured.x) / 2.0, (rateTargetVelocity[target].z + measured.z) / 2.0 };
			}
			rateTargetLast[target] = centre;
			const auto latencyMs = inherited_value(plan, setpoint.chain, static_cast<std::uint16_t>(DDI::PhysicalSetpointTimeLatency), received);
			const double latencyS = latencyMs ? std::clamp(static_cast<double>(*latencyMs) / 1000.0, 0.0, MAX_LATENCY_S) : 0.0;
			rateTargetPoints[target] = { centre.x + (rateTargetVelocity[target].x * latencyS), centre.z + (rateTargetVelocity[target].z * latencyS) };
		}

		// What each target should get from its group's source.
		std::fill(rateWanted.begin(), rateWanted.end(), std::nullopt);
		std::fill(rateWantedSource.begin(), rateWantedSource.end(), PrescriptionSource::None);
		std::vector<bool> engage(plan.channels.size(), false);
		const bool active = running && connected && taskActive;
		for (std::size_t group = 0; group < plan.groups.size(); ++group)
		{
			const auto &rateGroup = plan.groups[group];
			const int source = rateGroupSource(group);
			if (1 == source)
			{
				const auto choice = rateGroupChoices.find({ plan.channels[rateGroup.channel].element, rateGroup.ddi, rateGroup.bin ? *rateGroup.bin : -1 });
				const int fixed = (choice != rateGroupChoices.end()) ? choice->second.fixedValue : 0;
				for (const auto target : rateGroup.fixed_targets()) rateWanted[target] = fixed;
				engage[rateGroup.channel] = engage[rateGroup.channel] || active;
			}
			else if ((source >= 2) && (nullptr != map))
			{
				const auto layer = static_cast<std::size_t>(source - 2);
				for (const auto target : rateGroup.map_targets())
				{
					const auto point = rateTargetPoints[target];
					PrescriptionLookup lookup;
					if (!currentGps.valid)
					{
						lookup = map->value_without_position(layer);
					}
					else
					{
						const bool inside = (boundaryLocal.size() < 3) || point_in_ring(point, boundaryLocal);
						lookup = map->value_at(layer, localToGeo(point), inside);
					}
					rateWanted[target] = lookup.value;
					rateWantedSource[target] = lookup.source;
				}
				engage[rateGroup.channel] = engage[rateGroup.channel] || active;
			}
		}

		std::vector<bool> before(plan.channels.size());
		for (std::size_t channel = 0; channel < before.size(); ++channel) before[channel] = rateController.engaged(channel);
		const auto transition = rateController.set_engaged(engage, nowMs);
		if (!transition.empty() || (before != engage))
		{
			sendTcCommands(transition);
			for (std::size_t channel = 0; channel < engage.size(); ++channel)
			{
				if (before[channel] == engage[channel]) continue;
				const auto &rateChannel = plan.channels[channel];
				logs.addLine(engage[channel] ? QString("[rate] Rate control engaged: client %1 element %2, prescription control state automatic.").arg(planClient).arg(rateChannel.element)
				                             : QString("[rate] Rate control released: client %1 element %2, prescription control state manual.").arg(planClient).arg(rateChannel.element));
			}
		}
		sendTcCommands(rateController.update(rateWanted, nowMs));
		publishRateChannels(nowMs, false);
	}

	void TcBridge::setRateGroupSource(int group, int source)
	{
		const auto &plan = rateController.plan();
		if ((group < 0) || (group >= static_cast<int>(plan.groups.size()))) return;
		const auto &rateGroup = plan.groups[static_cast<std::size_t>(group)];
		auto &choice = rateGroupChoices[{ plan.channels[rateGroup.channel].element, rateGroup.ddi, rateGroup.bin ? *rateGroup.bin : -1 }];
		choice.source = std::max(0, source);
		QString what = (0 == choice.source) ? QString("off") : QString("fixed rate %1").arg(choice.fixedValue);
		if (choice.source >= 2)
		{
			const auto *map = selectedPrescription();
			const auto layer = static_cast<std::size_t>(choice.source - 2);
			what = ((nullptr != map) && (layer < map->layers().size())) ? QString("prescription layer %1").arg(QString::fromStdString(map->layers()[layer].describe()))
			                                                              : QString("a prescription layer");
		}
		logs.addLine(QString("[rate] %1 (DDI %2) on element %3: %4.").arg(rate_ddi_name(rateGroup.ddi)).arg(rateGroup.ddi).arg(plan.channels[rateGroup.channel].element).arg(what));
		serviceRateControl(steady_clock_ms());
		publishRateChannels(steady_clock_ms(), true);
	}

	void TcBridge::setRateGroupFixed(int group, int value)
	{
		const auto &plan = rateController.plan();
		if ((group < 0) || (group >= static_cast<int>(plan.groups.size()))) return;
		const auto &rateGroup = plan.groups[static_cast<std::size_t>(group)];
		auto &choice = rateGroupChoices[{ plan.channels[rateGroup.channel].element, rateGroup.ddi, rateGroup.bin ? *rateGroup.bin : -1 }];
		choice.fixedValue = std::max(0, value);
		publishRateChannels(steady_clock_ms(), true);
	}

	void TcBridge::publishRateChannels(std::uint64_t nowMs, bool force)
	{
		if (!force && ((nowMs - lastRatePublishMs) < RATE_PUBLISH_MS)) return;
		lastRatePublishMs = nowMs;
		const auto &plan = rateController.plan();
		const auto *map = selectedPrescription();
		const auto &commanded = rateController.commanded();
		auto received = [this](std::uint16_t element, std::uint16_t ddi) { return receivedValue(element, ddi); };
		auto presentation = [this](std::uint16_t ddi, std::uint16_t element) {
			for (const auto &state : implementDdiStates)
			{
				if ((state.ddi == ddi) && (state.element == element)) return std::make_tuple(state.displayOffset, state.displayScale, state.unit);
			}
			return std::make_tuple(0.0, 1.0, QString());
		};

		QVariantList channels;
		QVariantMap live;
		QVariantList markers;
		int engagedCount = 0;
		int sourcedGroups = 0;
		for (std::size_t channelIndex = 0; channelIndex < plan.channels.size(); ++channelIndex)
		{
			const auto &channel = plan.channels[channelIndex];
			QVariantMap channelRow;
			channelRow["index"] = static_cast<int>(channelIndex);
			channelRow["element"] = channel.element;
			channelRow["name"] = QString::fromStdString(channel.name).trimmed();
			channelRow["hasState"] = channel.hasPrescriptionControlState;
			if (rateController.engaged(channelIndex)) ++engagedCount;
			const std::vector<std::uint16_t> channelChain = channel.groups.empty() ? std::vector<std::uint16_t>{ channel.element }
			                                                                       : plan.targets[plan.groups[channel.groups.front()].map_targets().front()].chain;
			QVariantMap channelLive;
			channelLive["engaged"] = rateController.engaged(channelIndex);
			channelLive["state"] = state_text(receivedValue(channel.element, static_cast<std::uint16_t>(DDI::PrescriptionControlState)));
			channelLive["latencyMs"] = optional_value(inherited_value(plan, channelChain, static_cast<std::uint16_t>(DDI::PhysicalSetpointTimeLatency), received));
			channelLive["practice"] = optional_value(inherited_value(plan, channelChain, static_cast<std::uint16_t>(DDI::ActualCulturalPractice), received));
			live[QString("c%1").arg(channelIndex)] = channelLive;

			QVariantList groups;
			for (const auto groupIndex : channel.groups)
			{
				const auto &group = plan.groups[groupIndex];
				const int source = rateGroupSource(groupIndex);
				if (0 != source) ++sourcedGroups;
				const auto choice = rateGroupChoices.find({ channel.element, group.ddi, group.bin ? *group.bin : -1 });
				const auto display = presentation(group.ddi, group.top ? plan.targets[*group.top].element : plan.targets[group.subs.front()].element);
				const auto actualDdi = actual_rate_ddi(group.ddi);
				std::optional<std::size_t> layer;
				if ((source >= 2) && (nullptr != map)) layer = static_cast<std::size_t>(source - 2);
				const auto matched = matchedLayer(groupIndex);
				const auto mapTargets = group.map_targets();

				auto targetRow = [&](std::size_t target) {
					const auto &setpoint = plan.targets[target];
					QVariantMap row;
					row["target"] = static_cast<int>(target);
					row["element"] = setpoint.element;
					row["name"] = QString::fromStdString(setpoint.name).trimmed();
					QVariantMap values;
					values["commanded"] = optional_value((target < commanded.size()) ? commanded[target] : std::nullopt);
					values["wanted"] = optional_value(rateWanted[target]);
					const bool mapped = layer && (std::find(mapTargets.cbegin(), mapTargets.cend(), target) != mapTargets.cend());
					values["source"] = mapped ? source_text(rateWantedSource[target], 0) : QString();
					values["actual"] = (0 != actualDdi) ? optional_value(receivedValue(setpoint.element, actualDdi)) : QVariant();
					live[QString("t%1").arg(target)] = values;
					return row;
				};
				QVariantMap groupRow;
				groupRow["index"] = static_cast<int>(groupIndex);
				groupRow["ddi"] = group.ddi;
				groupRow["name"] = rate_ddi_name(group.ddi);
				groupRow["bin"] = group.bin ? QVariant(*group.bin) : QVariant();
				groupRow["source"] = source;
				groupRow["automatic"] = (choice == rateGroupChoices.end()) || (choice->second.source < 0);
				groupRow["fixed"] = (choice != rateGroupChoices.end()) ? choice->second.fixedValue : 0;
				groupRow["matchedLayer"] = matched ? static_cast<int>(*matched) : -1;
				groupRow["displayOffset"] = std::get<0>(display);
				groupRow["displayScale"] = std::get<1>(display);
				groupRow["unit"] = std::get<2>(display);
				groupRow["top"] = group.top ? QVariant(targetRow(*group.top)) : QVariant();
				QVariantList subs;
				for (const auto sub : group.subs) subs.push_back(targetRow(sub));
				groupRow["subs"] = subs;
				groups.push_back(groupRow);

				// Markers where the map is looked up, coloured on the layer's scale.
				if (layer && currentGps.valid && implementReadyFlag)
				{
					const auto &mapLayer = map->layers()[*layer];
					const double span = std::max(1.0, static_cast<double>(mapLayer.maximum) - mapLayer.minimum);
					for (const auto target : group.map_targets())
					{
						if (!rateWanted[target]) continue;
						const auto element = std::find_if(implementElementStates.cbegin(), implementElementStates.cend(),
						                                  [&](const ImplementElementState &state) { return state.element == plan.targets[target].element; });
						QVariantMap marker;
						marker["x"] = rateTargetPoints[target].x;
						marker["z"] = rateTargetPoints[target].z;
						marker["width"] = (element != implementElementStates.cend()) ? std::max(0.5, element->width) : 1.0;
						marker["colour"] = rate_colour((static_cast<double>(*rateWanted[target]) - mapLayer.minimum) / span);
						marker["label"] = QString::number((static_cast<double>(*rateWanted[target]) + std::get<0>(display)) * std::get<1>(display), 'f', 1);
						markers.push_back(marker);
					}
				}
			}
			channelRow["groups"] = groups;
			channels.push_back(channelRow);
		}
		currentRateLive = live;
		rateMarkerRows.setRows(markers);

		QString status;
		if (planClient < 0) status = "No client";
		else if (plan.targets.empty()) status = "The client's DDOP offers no rate setpoints";
		else if (engagedCount > 0) status = QString("Controlling %1 of %2 channel(s)").arg(engagedCount).arg(plan.channels.size());
		else if (sourcedGroups > 0) status = QString("Ready: starts with the task (%1 rate(s) with a source)").arg(sourcedGroups);
		else status = QString("Ready: choose a rate source (%1 channel(s))").arg(plan.channels.size());
		// The structure goes to the views only when it changed, so their controls stay as they are.
		if ((channels != currentRateChannels) || (status != currentRateControlStatus))
		{
			currentRateChannels = channels;
			currentRateControlStatus = status;
			emit rateControlChanged();
		}
		emit rateLiveChanged();
	}

	// --- the prescription -----------------------------------------------------------------------

	const Prescription *TcBridge::selectedPrescription() const
	{
		if ((currentSelectedTask < 0) || (currentSelectedTask >= static_cast<int>(taskIds.size()))) return nullptr;
		const auto found = taskPrescriptions.find(taskIds[static_cast<std::size_t>(currentSelectedTask)]);
		return ((found == taskPrescriptions.end()) || found->second.empty()) ? nullptr : &found->second;
	}

	Prescription *TcBridge::editablePrescription()
	{
		if ((currentSelectedTask < 0) || (currentSelectedTask >= static_cast<int>(taskIds.size()))) return nullptr;
		auto &prescription = taskPrescriptions[taskIds[static_cast<std::size_t>(currentSelectedTask)]];
		if (prescription.name.empty()) prescription.name = currentTaskNames.value(currentSelectedTask).toStdString();
		return &prescription;
	}

	void TcBridge::refreshPrescription(bool rebuildPlan)
	{
		// The layers first: the rate groups' sources refer to them.
		renderPrescription();
		if (rebuildPlan)
		{
			isobus::DeviceDescriptorObjectPool pool;
			bool parsed = false;
			if ((nullptr != server) && (planClient >= 0))
			{
				const auto address = static_cast<std::uint8_t>(planClient);
				parsed = (0 != parse_client_pool(server->stored_pool(address), server->client_version(address), pool));
			}
			setupRatePlan(parsed ? &pool : nullptr, true);
		}
		publishRateChannels(steady_clock_ms(), true);
	}

	void TcBridge::renderPrescription()
	{
		QVariantMap info;
		const auto *map = selectedPrescription();
		info["present"] = (nullptr != map);
		if (nullptr == map)
		{
			prescriptionImages->set(QImage());
			info["imageUrl"] = QString();
			info["layers"] = QVariantList();
			currentPrescription = info;
			emit prescriptionChanged();
			return;
		}
		const auto &layers = map->layers();
		currentPrescriptionLayer = std::clamp(currentPrescriptionLayer, 0, static_cast<int>(layers.size()) - 1);
		const auto &shown = layers[static_cast<std::size_t>(currentPrescriptionLayer)];

		// Values are shown as the map's value presentation gives them, else as the client's does.
		ValuePresentation presentation;
		if (shown.presentation)
		{
			presentation = *shown.presentation;
		}
		else
		{
			for (const auto &state : implementDdiStates)
			{
				if (state.ddi != shown.ddi) continue;
				presentation.offset = static_cast<std::int32_t>(state.displayOffset);
				presentation.scale = state.displayScale;
				presentation.unit = state.unit.toStdString();
				presentation.decimals = (state.displayScale < 1.0) ? 1 : 0;
				break;
			}
		}
		auto shownValue = [&presentation](std::int32_t raw) {
			return QString::number((static_cast<double>(raw) + presentation.offset) * presentation.scale, 'f', std::clamp(presentation.decimals, 0, 6));
		};

		QVariantList layerRows;
		for (std::size_t index = 0; index < layers.size(); ++index)
		{
			QVariantMap row;
			row["index"] = static_cast<int>(index);
			row["ddi"] = layers[index].ddi;
			row["name"] = QString("%1 (%2)").arg(rate_ddi_name(layers[index].ddi), QString::fromStdString(layers[index].describe()));
			row["minimum"] = static_cast<qlonglong>(layers[index].minimum);
			row["maximum"] = static_cast<qlonglong>(layers[index].maximum);
			layerRows.push_back(row);
		}
		info["name"] = QString::fromStdString(map->name);
		info["description"] = QString::fromStdString(map->describe());
		info["layers"] = layerRows;
		info["selectedLayer"] = currentPrescriptionLayer;
		info["legendMinimum"] = shownValue(shown.minimum);
		info["legendMaximum"] = shownValue(shown.maximum);
		info["legendUnit"] = QString::fromStdString(presentation.unit);
		info["legendRaw"] = presentation.unit.empty() && (1.0 == presentation.scale);

		GeoPoint southWest;
		GeoPoint northEast;
		if (!fieldOriginValid || !map->bounds(southWest, northEast))
		{
			prescriptionImages->set(QImage());
			info["imageUrl"] = QString();
			currentPrescription = info;
			emit prescriptionChanged();
			return;
		}
		const auto corner = geoToLocal(southWest);
		const auto opposite = geoToLocal(northEast);
		const double west = corner.x;
		const double south = corner.z;
		const double east = opposite.x;
		const double north = opposite.z; // z grows to the south
		const double widthM = std::max(0.1, east - west);
		const double heightM = std::max(0.1, south - north);
		const auto layer = static_cast<std::size_t>(currentPrescriptionLayer);
		const double span = std::max(1.0, static_cast<double>(shown.maximum) - shown.minimum);
		auto colourOf = [&](std::int32_t value) {
			QColor colour = rate_colour((static_cast<double>(value) - shown.minimum) / span);
			colour.setAlpha(225);
			return colour;
		};

		// A grid alone is drawn cell by cell; zones are painted at a resolution of their own.
		const bool polygons = std::any_of(map->zones.cbegin(), map->zones.cend(), [](const TreatmentZone &zone) { return !zone.polygons.empty(); });
		int pixelsWide = 0;
		int pixelsHigh = 0;
		if (map->grid && !polygons && (map->grid->columns <= 2048) && (map->grid->rows <= 2048))
		{
			pixelsWide = static_cast<int>(std::max<std::uint32_t>(1, map->grid->columns));
			pixelsHigh = static_cast<int>(std::max<std::uint32_t>(1, map->grid->rows));
		}
		else
		{
			const double metresPerPixel = std::max(widthM, heightM) / MAP_IMAGE_PIXELS;
			pixelsWide = std::clamp(static_cast<int>(std::ceil(widthM / metresPerPixel)), 1, 2048);
			pixelsHigh = std::clamp(static_cast<int>(std::ceil(heightM / metresPerPixel)), 1, 2048);
		}
		QImage image(pixelsWide, pixelsHigh, QImage::Format_ARGB32);
		image.fill(Qt::transparent);
		if (map->defaultZone)
		{
			if (const auto value = map->zone_value(*map->defaultZone, layer)) image.fill(colourOf(*value));
		}
		if (map->grid)
		{
			for (int y = 0; y < pixelsHigh; ++y)
			{
				const double z = north + ((y + 0.5) * heightM / pixelsHigh); // row 0 is the north edge
				for (int x = 0; x < pixelsWide; ++x)
				{
					const auto value = map->grid_value(layer, localToGeo({ west + ((x + 0.5) * widthM / pixelsWide), z }));
					if (value) image.setPixelColor(x, y, colourOf(*value));
				}
			}
		}
		if (polygons)
		{
			QPainter painter(&image);
			painter.setPen(Qt::NoPen);
			painter.setCompositionMode(QPainter::CompositionMode_Source);
			auto toPixel = [&](GeoPoint point) {
				const auto local = geoToLocal(point);
				return QPointF((local.x - west) * pixelsWide / widthM, (local.z - north) * pixelsHigh / heightM);
			};
			// The first zone wins where zones overlap, so it is painted last.
			for (auto zone = map->zones.crbegin(); zone != map->zones.crend(); ++zone)
			{
				const auto value = map->zone_value(zone->code, layer);
				if (!value || zone->polygons.empty()) continue;
				QPainterPath path;
				path.setFillRule(Qt::OddEvenFill);
				for (const auto &polygon : zone->polygons)
				{
					QPolygonF ring;
					for (const auto &point : polygon.exterior) ring << toPixel(point);
					path.addPolygon(ring);
					path.closeSubpath();
					for (const auto &hole : polygon.holes)
					{
						QPolygonF holeRing;
						for (const auto &point : hole) holeRing << toPixel(point);
						path.addPolygon(holeRing);
						path.closeSubpath();
					}
				}
				painter.fillPath(path, colourOf(*value));
			}
		}
		prescriptionImages->set(image);
		++prescriptionImageSerial;
		info["imageUrl"] = QString("image://prescription/%1").arg(prescriptionImageSerial);
		info["centreX"] = (west + east) / 2.0;
		info["centreZ"] = (north + south) / 2.0;
		info["width"] = widthM;
		info["height"] = heightM;
		currentPrescription = info;
		emit prescriptionChanged();
	}

	void TcBridge::selectPrescriptionLayer(int index)
	{
		currentPrescriptionLayer = std::max(0, index);
		renderPrescription();
	}

	bool TcBridge::createTestPrescription(int ddi, int pattern, int rateA, int rateB, double cellSizeM, double patternSizeM)
	{
		if ((currentSelectedTask < 0) || (currentSelectedTask >= static_cast<int>(taskIds.size())))
		{
			setStatus("Select or create a task first: the map belongs to a task.");
			return false;
		}
		const auto task = fieldTaskManager.get_task(taskIds[static_cast<std::size_t>(currentSelectedTask)]);
		const auto field = task ? fieldTaskManager.get_field(task->fieldId) : std::nullopt;
		if (!field || (field->exteriorRing.size() < 3))
		{
			setStatus("The task's field has no boundary to lay the map over.");
			return false;
		}
		if ((ddi <= 0) || (ddi > 0xFFFF))
		{
			setStatus("Choose the DDI of the rate the map is for.");
			return false;
		}
		GeoPoint southWest = { field->exteriorRing.front().first, field->exteriorRing.front().second };
		GeoPoint northEast = southWest;
		for (const auto &[latitude, longitude] : field->exteriorRing)
		{
			southWest = { std::min(southWest.latitude, latitude), std::min(southWest.longitude, longitude) };
			northEast = { std::max(northEast.latitude, latitude), std::max(northEast.longitude, longitude) };
		}
		auto *map = editablePrescription();
		PrescriptionValue layer;
		layer.ddi = static_cast<std::uint16_t>(ddi);
		const auto shape = static_cast<TestPattern>(std::clamp(pattern, 0, 3));
		add_test_layer(*map, southWest, northEast, cellSizeM, layer, shape, rateA, rateB, patternSizeM);
		static const char *const PATTERNS[] = { "checkerboard", "stripes", "bands", "gradient" };
		logs.addLine(QString("[rate] Test map for task %1: %2 (DDI %3), %4 of %5 and %6, %7 m cells: %8.")
		               .arg(currentTaskNames.value(currentSelectedTask))
		               .arg(rate_ddi_name(layer.ddi))
		               .arg(ddi)
		               .arg(PATTERNS[static_cast<int>(shape)])
		               .arg(rateA)
		               .arg(rateB)
		               .arg(cellSizeM, 0, 'f', 1)
		               .arg(QString::fromStdString(map->describe())));
		const auto &layers = map->layers();
		for (std::size_t index = 0; index < layers.size(); ++index)
		{
			if (layers[index].matches(layer)) currentPrescriptionLayer = static_cast<int>(index);
		}
		refreshPrescription(true);
		setStatus(QString("Test map added to task %1.").arg(currentTaskNames.value(currentSelectedTask)));
		return true;
	}

	bool TcBridge::addRateZone(const QVariantList &points, int ddi, int value)
	{
		if ((points.size() < 3) || !fieldOriginValid || (ddi <= 0) || (ddi > 0xFFFF)) return false;
		auto *map = editablePrescription();
		if (nullptr == map)
		{
			setStatus("Select or create a task first: the zone belongs to its map.");
			return false;
		}
		std::vector<GeoPoint> ring;
		for (const auto &entry : points)
		{
			const auto point = entry.toMap();
			ring.push_back(localToGeo({ point.value("x").toDouble(), point.value("z").toDouble() }));
		}
		ring.push_back(ring.front());
		PrescriptionValue rate;
		rate.ddi = static_cast<std::uint16_t>(ddi);
		rate.value = value;
		const auto code = add_polygon_zone(*map, ring, rate, QString("Zone %1").arg(map->zones.size() + 1).toStdString());
		logs.addLine(QString("[rate] Zone %1 drawn for task %2: %3 (DDI %4) = %5.")
		               .arg(code)
		               .arg(currentTaskNames.value(currentSelectedTask))
		               .arg(rate_ddi_name(rate.ddi))
		               .arg(ddi)
		               .arg(value));
		refreshPrescription(true);
		return true;
	}

	void TcBridge::clearPrescription()
	{
		if ((currentSelectedTask < 0) || (currentSelectedTask >= static_cast<int>(taskIds.size()))) return;
		if (0 == taskPrescriptions.erase(taskIds[static_cast<std::size_t>(currentSelectedTask)])) return;
		logs.addLine(QString("[rate] Map of task %1 removed.").arg(currentTaskNames.value(currentSelectedTask)));
		refreshPrescription(true);
	}

	void TcBridge::importTaskData(const QUrl &fileUrl)
	{
		const QFileInfo info(fileUrl.toLocalFile());
		QFile file(info.absoluteFilePath());
		if (!file.open(QIODevice::ReadOnly))
		{
			setStatus("Could not open the task data file.");
			return;
		}
		const QByteArray xml = file.readAll();
		// The grid and external files sit next to TASKDATA.XML; their names' case varies.
		const QDir folder = info.absoluteDir();
		const QStringList names = folder.entryList(QDir::Files);
		auto reader = [&](const std::string &name) -> std::optional<std::vector<std::uint8_t>> {
			for (const auto &candidate : names)
			{
				if (0 != candidate.compare(QString::fromStdString(name), Qt::CaseInsensitive)) continue;
				QFile part(folder.filePath(candidate));
				if (!part.open(QIODevice::ReadOnly)) return std::nullopt;
				const QByteArray bytes = part.readAll();
				return std::vector<std::uint8_t>(bytes.cbegin(), bytes.cend());
			}
			return std::nullopt;
		};
		TaskDataImport imported;
		std::string error;
		if (!import_task_data(std::string_view(xml.constData(), static_cast<std::size_t>(xml.size())), reader, imported, error))
		{
			setStatus(QString("Task data not imported: %1").arg(QString::fromStdString(error)));
			logs.addLine(QString("[task] %1 not imported: %2").arg(info.fileName(), QString::fromStdString(error)));
			return;
		}

		std::map<std::string, std::string> fieldByPartfield;
		for (const auto &partfield : imported.fields)
		{
			if (partfield.boundary.size() < 3) continue;
			FieldBoundary field;
			field.name = partfield.name;
			for (const auto &point : partfield.boundary) field.exteriorRing.emplace_back(point.latitude, point.longitude);
			if (field.exteriorRing.front() != field.exteriorRing.back()) field.exteriorRing.push_back(field.exteriorRing.front());
			const auto id = fieldTaskManager.add_field(field);
			if (!id.empty()) fieldByPartfield[partfield.id] = id;
		}
		int taskCount = 0;
		int mapCount = 0;
		std::string firstTask;
		for (const auto &importedTask : imported.tasks)
		{
			std::string fieldId;
			const auto partfield = fieldByPartfield.find(importedTask.partfieldId);
			GeoPoint southWest;
			GeoPoint northEast;
			if (partfield != fieldByPartfield.end())
			{
				fieldId = partfield->second;
			}
			else if ((currentSelectedField >= 0) && (currentSelectedField < static_cast<int>(fieldIds.size())))
			{
				fieldId = fieldIds[static_cast<std::size_t>(currentSelectedField)];
			}
			else if (importedTask.prescription.bounds(southWest, northEast))
			{
				// No field to put it on: the area the map covers becomes one.
				FieldBoundary field;
				field.name = importedTask.name + " area";
				field.exteriorRing = { { southWest.latitude, southWest.longitude }, { southWest.latitude, northEast.longitude },
					                   { northEast.latitude, northEast.longitude }, { northEast.latitude, southWest.longitude },
					                   { southWest.latitude, southWest.longitude } };
				fieldId = fieldTaskManager.add_field(field);
			}
			if (fieldId.empty())
			{
				logs.addLine(QString("[task] %1 skipped: no field for it.").arg(QString::fromStdString(importedTask.name)));
				continue;
			}
			Task task;
			task.name = importedTask.name;
			task.fieldId = fieldId;
			const auto taskId = fieldTaskManager.create_task(task);
			if (taskId.empty()) continue;
			++taskCount;
			if (firstTask.empty()) firstTask = taskId;
			if (!importedTask.prescription.empty())
			{
				taskPrescriptions[taskId] = importedTask.prescription;
				++mapCount;
			}
			logs.addLine(QString("[task] Imported %1: %2.")
			               .arg(QString::fromStdString(importedTask.name))
			               .arg(importedTask.prescription.empty() ? QString("no prescription") : QString::fromStdString(importedTask.prescription.describe())));
		}
		for (const auto &warning : imported.warnings)
		{
			logs.addLine(QString("[task] %1: %2").arg(info.fileName(), QString::fromStdString(warning)));
		}
		refreshFieldNames();
		refreshTaskNames();
		const auto selected = std::find(taskIds.begin(), taskIds.end(), firstTask);
		if (selected != taskIds.end()) selectTask(static_cast<int>(std::distance(taskIds.begin(), selected)));
		setStatus(QString("Imported %1 task(s), %2 with a prescription, from %3.").arg(taskCount).arg(mapCount).arg(info.fileName()));
	}
} // namespace agisotc
