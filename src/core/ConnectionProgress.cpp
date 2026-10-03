#include "ConnectionProgress.hpp"

#include <algorithm>

namespace agisotc
{
	namespace
	{
		constexpr std::uint32_t PROCESS_DATA_PGN = 0xCB00;
		constexpr std::uint32_t TP_CONNECTION_PGN = 0xEC00;
		constexpr std::uint32_t TP_DATA_PGN = 0xEB00;
		constexpr std::uint32_t ETP_CONNECTION_PGN = 0xC800;
		constexpr std::uint32_t ETP_DATA_PGN = 0xC700;
		constexpr std::uint8_t TP_REQUEST_TO_SEND = 16;
		constexpr std::uint8_t ETP_REQUEST_TO_SEND = 20;
		constexpr std::uint8_t REQUEST_OBJECT_POOL_TRANSFER = 0x41;
		constexpr std::uint32_t BYTES_PER_PACKET = 7;
	} // namespace

	bool ConnectionProgress::is_implement_name(std::uint64_t name)
	{
		const auto industryGroup = static_cast<unsigned>((name >> 60) & 0x07);
		const auto deviceClass = static_cast<unsigned>((name >> 49) & 0x7F);
		return (2 == industryGroup) && (deviceClass >= 2);
	}

	void ConnectionProgress::reset()
	{
		entries.clear();
	}

	ConnectionProgress::Entry &ConnectionProgress::start(std::uint8_t address, std::uint64_t nowMs)
	{
		Entry &entry = entries[address];
		entry = Entry();
		entry.appearedMs = nowMs;
		entry.progressMs = nowMs;
		return entry;
	}

	void ConnectionProgress::advance(Entry &entry, Step step, std::uint64_t nowMs)
	{
		if (step <= entry.step)
		{
			return;
		}
		switch (step)
		{
			case Step::Connecting: entry.connectingMs = nowMs; break;
			case Step::Uploading: entry.uploadingMs = nowMs; break;
			case Step::Building: entry.buildingMs = nowMs; break;
			case Step::Ready: entry.readyMs = nowMs; break;
			default: break;
		}
		entry.step = step;
		entry.progressMs = nowMs;
	}

	void ConnectionProgress::on_address_claim(std::uint8_t address, std::uint64_t name, std::uint64_t nowMs)
	{
		// Every device claims again when any device asks for the claims, so a claim alone does not
		// restart an implement that is connecting or built; its time-out does (on_client_lost).
		if (is_implement_name(name) && (entries.find(address) == entries.end()))
		{
			Entry &entry = start(address, nowMs);
			entry.step = Step::StartingUp;
			entry.startUpSeenMs = nowMs;
		}
	}

	void ConnectionProgress::on_working_set_master(std::uint8_t address, std::uint64_t nowMs)
	{
		if (entries.find(address) == entries.end())
		{
			Entry &entry = start(address, nowMs);
			entry.step = Step::StartingUp;
			entry.startUpSeenMs = nowMs;
		}
	}

	void ConnectionProgress::on_frame_to_tc(std::uint8_t source, std::uint32_t pgn, const std::uint8_t *data, std::uint8_t length, std::uint64_t nowMs)
	{
		if ((nullptr == data) || (0 == length))
		{
			return;
		}
		auto found = entries.find(source);

		// The DDOP arrives in an object pool transfer: the client asks for it (a single process data
		// frame with the pool's size), then sends it with the transport protocol (TP up to 1785
		// bytes, else ETP). Its data packets are counted while that session is under way.
		if ((TP_DATA_PGN == pgn) || (ETP_DATA_PGN == pgn))
		{
			if ((found != entries.end()) && found->second.transferOpen)
			{
				Entry &entry = found->second;
				entry.receivedBytes = std::min(entry.transferBytes, entry.receivedBytes + BYTES_PER_PACKET);
				entry.progressMs = nowMs;
				entry.transferOpen = (entry.receivedBytes < entry.transferBytes);
			}
			return;
		}
		if ((length >= 8) &&
		    (((TP_CONNECTION_PGN == pgn) && (TP_REQUEST_TO_SEND == data[0])) ||
		     ((ETP_CONNECTION_PGN == pgn) && (ETP_REQUEST_TO_SEND == data[0]))))
		{
			// Only the session that follows the request carries the pool; others (a designator change,
			// say) are messages of a client that is connected already.
			const std::uint32_t transported = static_cast<std::uint32_t>(data[5]) | (static_cast<std::uint32_t>(data[6]) << 8) |
			  (static_cast<std::uint32_t>(data[7]) << 16);
			if ((PROCESS_DATA_PGN != transported) || (found == entries.end()) || !found->second.transferRequested)
			{
				return;
			}
			Entry &entry = found->second;
			std::uint32_t bytes = static_cast<std::uint32_t>(data[1]) | (static_cast<std::uint32_t>(data[2]) << 8);
			if (ETP_CONNECTION_PGN == pgn)
			{
				bytes |= (static_cast<std::uint32_t>(data[3]) << 16) | (static_cast<std::uint32_t>(data[4]) << 24);
			}
			entry.transferRequested = false;
			entry.transferOpen = true;
			entry.transferBytes = bytes;
			entry.receivedBytes = 0;
			entry.progressMs = nowMs;
			return;
		}
		if (PROCESS_DATA_PGN != pgn)
		{
			return;
		}

		if (found == entries.end())
		{
			start(source, nowMs);
			found = entries.find(source);
		}
		Entry &entry = found->second;
		if ((length >= 5) && (REQUEST_OBJECT_POOL_TRANSFER == data[0]))
		{
			if (entry.step >= Step::Building)
			{
				// A client that was built uploads a new pool: it connects again.
				start(source, nowMs);
			}
			advance(entry, Step::Connecting, nowMs);
			advance(entry, Step::Uploading, nowMs);
			entry.uploadedBytes += entry.receivedBytes;
			entry.transferBytes = static_cast<std::uint32_t>(data[1]) | (static_cast<std::uint32_t>(data[2]) << 8) |
			  (static_cast<std::uint32_t>(data[3]) << 16) | (static_cast<std::uint32_t>(data[4]) << 24);
			entry.receivedBytes = 0;
			entry.transferRequested = true;
			entry.transferOpen = false;
			entry.progressMs = nowMs;
			return;
		}
		advance(entry, Step::Connecting, nowMs);
	}

	void ConnectionProgress::on_pool_activated(std::uint8_t address, std::uint64_t nowMs)
	{
		auto found = entries.find(address);
		if (found == entries.end())
		{
			start(address, nowMs);
			found = entries.find(address);
		}
		Entry &entry = found->second;
		if (Step::Ready == entry.step)
		{
			// Activated again (e.g. after a deactivation): it is built again.
			entry.step = Step::Uploading;
			entry.completionTaken = false;
		}
		entry.uploadedBytes += entry.receivedBytes;
		entry.receivedBytes = 0;
		entry.transferRequested = false;
		entry.transferOpen = false;
		entry.geometryReceived = 0;
		entry.geometryTotal = -1;
		advance(entry, Step::Building, nowMs);
	}

	void ConnectionProgress::on_geometry(std::uint8_t address, int received, int total, std::uint64_t nowMs)
	{
		auto found = entries.find(address);
		if ((found == entries.end()) || (Step::Building != found->second.step))
		{
			return;
		}
		Entry &entry = found->second;
		entry.geometryReceived = std::max(0, received);
		entry.geometryTotal = std::max(0, total);
		if (entry.geometryReceived >= entry.geometryTotal)
		{
			advance(entry, Step::Ready, nowMs);
		}
	}

	void ConnectionProgress::on_client_lost(std::uint8_t address)
	{
		entries.erase(address);
	}

	double ConnectionProgress::progress_of(const Entry &entry, std::uint64_t nowMs)
	{
		switch (entry.step)
		{
			case Step::StartingUp:
			{
				const double waited = static_cast<double>(nowMs - entry.appearedMs) / static_cast<double>(EXPECTED_STARTUP_MS);
				return 0.02 + (0.33 * std::min(1.0, waited));
			}
			case Step::Connecting:
				return 0.4;
			case Step::Uploading:
				return 0.45 + (0.4 * ((entry.transferBytes > 0) ? static_cast<double>(entry.receivedBytes) / entry.transferBytes : 0.0));
			case Step::Building:
				return 0.88 + (0.12 * ((entry.geometryTotal > 0) ? static_cast<double>(entry.geometryReceived) / entry.geometryTotal : 0.0));
			case Step::Ready:
				return 1.0;
			default:
				return 0.0;
		}
	}

	ConnectionProgress::State ConnectionProgress::current(std::uint64_t nowMs)
	{
		for (auto it = entries.begin(); it != entries.end();)
		{
			Entry &entry = it->second;
			if ((Step::StartingUp == entry.step) && ((nowMs - entry.appearedMs) > STARTUP_GIVE_UP_MS))
			{
				it = entries.erase(it);
				continue;
			}
			if (((Step::Connecting == entry.step) || (Step::Uploading == entry.step)) &&
			    ((nowMs - entry.progressMs) > STALL_GIVE_UP_MS))
			{
				it = entries.erase(it);
				continue;
			}
			if ((Step::Building == entry.step) && ((nowMs - entry.buildingMs) > GEOMETRY_WAIT_MS))
			{
				advance(entry, Step::Ready, nowMs);
			}
			++it;
		}

		// The implement furthest along that is still loading; else one that just got ready.
		const Entry *best = nullptr;
		int bestAddress = -1;
		int bestRank = 0;
		for (const auto &[address, entry] : entries)
		{
			int rank = 0;
			if ((entry.step >= Step::StartingUp) && (entry.step <= Step::Building))
			{
				rank = 10 + static_cast<int>(entry.step);
			}
			else if ((Step::Ready == entry.step) && ((nowMs - entry.readyMs) <= READY_HOLD_MS))
			{
				rank = 1;
			}
			if ((rank > bestRank) || ((rank == bestRank) && (rank > 0) && (entry.appearedMs < best->appearedMs)))
			{
				best = &entry;
				bestAddress = address;
				bestRank = rank;
			}
		}

		State state;
		if (nullptr == best)
		{
			return state;
		}
		state.step = best->step;
		state.address = bestAddress;
		state.progress = progress_of(*best, nowMs);
		state.transferBytes = best->transferBytes;
		state.receivedBytes = best->receivedBytes;
		state.uploadSkipped = (0 == best->uploadingMs) && (best->step >= Step::Building);
		state.geometryReceived = std::max(0, best->geometryReceived);
		state.geometryTotal = std::max(0, best->geometryTotal);
		state.elapsedMs = nowMs - best->appearedMs;
		return state;
	}

	bool ConnectionProgress::is_ready(std::uint8_t address) const
	{
		const auto found = entries.find(address);
		return (found == entries.end()) || (Step::Ready == found->second.step);
	}

	bool ConnectionProgress::take_completed(Timings &timings)
	{
		for (auto &[address, entry] : entries)
		{
			if ((Step::Ready != entry.step) || entry.completionTaken)
			{
				continue;
			}
			entry.completionTaken = true;
			timings = Timings();
			timings.address = address;
			if ((0 != entry.startUpSeenMs) && (0 != entry.connectingMs))
			{
				timings.startUpMs = entry.connectingMs - entry.startUpSeenMs;
			}
			const std::uint64_t connectEnd = (0 != entry.uploadingMs) ? entry.uploadingMs : entry.buildingMs;
			if ((0 != entry.connectingMs) && (connectEnd >= entry.connectingMs))
			{
				timings.connectMs = connectEnd - entry.connectingMs;
			}
			if (0 != entry.uploadingMs)
			{
				timings.uploadMs = entry.buildingMs - entry.uploadingMs;
			}
			timings.buildMs = entry.readyMs - entry.buildingMs;
			timings.totalMs = entry.readyMs - entry.appearedMs;
			timings.uploadedBytes = entry.uploadedBytes;
			return true;
		}
		return false;
	}
} // namespace agisotc
