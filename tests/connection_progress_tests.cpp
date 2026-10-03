// Tests for following an implement from the bus to a built implement (ConnectionProgress).
#include "ConnectionProgress.hpp"

#include <cmath>
#include <cstdio>
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

	using agisotc::ConnectionProgress;
	using Step = ConnectionProgress::Step;

	constexpr std::uint8_t CLIENT = 128;
	/// A sprayer: agricultural industry group, device class 6, function 128.
	constexpr std::uint64_t SPRAYER_NAME = 0xA00C8000AFE00002ULL;
	/// A virtual terminal: agricultural industry group, device class 0, function 29.
	constexpr std::uint64_t TERMINAL_NAME = 0xA000000000000000ULL | (29ULL << 40);

	bool near(double value, double expected)
	{
		return std::fabs(value - expected) < 0.001;
	}

	void process_data(ConnectionProgress &progress, std::vector<std::uint8_t> data, std::uint64_t nowMs)
	{
		data.resize(8, 0xFF);
		progress.on_frame_to_tc(CLIENT, 0xCB00, data.data(), 8, nowMs);
	}

	void request_pool_transfer(ConnectionProgress &progress, std::uint32_t bytes, std::uint64_t nowMs)
	{
		process_data(progress, { 0x41, static_cast<std::uint8_t>(bytes), static_cast<std::uint8_t>(bytes >> 8),
		                         static_cast<std::uint8_t>(bytes >> 16), static_cast<std::uint8_t>(bytes >> 24) },
		             nowMs);
	}

	void etp_request_to_send(ConnectionProgress &progress, std::uint32_t bytes, std::uint64_t nowMs)
	{
		const std::uint8_t data[8] = { 20, static_cast<std::uint8_t>(bytes), static_cast<std::uint8_t>(bytes >> 8),
			                           static_cast<std::uint8_t>(bytes >> 16), static_cast<std::uint8_t>(bytes >> 24), 0x00, 0xCB, 0x00 };
		progress.on_frame_to_tc(CLIENT, 0xC800, data, 8, nowMs);
	}

	void etp_data(ConnectionProgress &progress, int packets, std::uint64_t nowMs)
	{
		const std::uint8_t data[8] = { 1, 0, 0, 0, 0, 0, 0, 0 };
		for (int i = 0; i < packets; ++i) progress.on_frame_to_tc(CLIENT, 0xC700, data, 8, nowMs);
	}

	void test_implement_names()
	{
		CHECK(ConnectionProgress::is_implement_name(SPRAYER_NAME));
		CHECK(!ConnectionProgress::is_implement_name(TERMINAL_NAME));

		ConnectionProgress progress;
		progress.on_address_claim(38, TERMINAL_NAME, 0);
		CHECK(Step::None == progress.current(100).step);
	}

	void test_full_connection()
	{
		ConnectionProgress progress;
		progress.on_address_claim(CLIENT, SPRAYER_NAME, 1000);
		auto state = progress.current(5000);
		CHECK(Step::StartingUp == state.step);
		CHECK(CLIENT == state.address);
		CHECK(near(state.progress, 0.02 + 0.33 * 0.5)); // half of the expected start-up
		CHECK(!progress.is_ready(CLIENT));

		process_data(progress, { 0x00 }, 8000); // asks for the TC's version
		CHECK(Step::Connecting == progress.current(8000).step);
		CHECK(near(progress.current(8000).progress, 0.4));

		request_pool_transfer(progress, 7000, 8100);
		etp_request_to_send(progress, 7001, 8150);
		etp_data(progress, 500, 9000); // 3500 bytes
		state = progress.current(9000);
		CHECK(Step::Uploading == state.step);
		CHECK(7001 == state.transferBytes);
		CHECK(3500 == state.receivedBytes);
		CHECK(near(state.progress, 0.45 + 0.4 * (3500.0 / 7001.0)));

		etp_data(progress, 600, 9500); // more than announced: capped
		CHECK(7001 == progress.current(9500).receivedBytes);

		progress.on_pool_activated(CLIENT, 10000);
		progress.on_geometry(CLIENT, 1, 4, 10100);
		state = progress.current(10100);
		CHECK(Step::Building == state.step);
		CHECK(!state.uploadSkipped);
		CHECK(near(state.progress, 0.88 + 0.12 * 0.25));
		CHECK(!progress.is_ready(CLIENT));

		progress.on_geometry(CLIENT, 4, 4, 10400);
		state = progress.current(10400);
		CHECK(Step::Ready == state.step);
		CHECK(near(state.progress, 1.0));
		CHECK(progress.is_ready(CLIENT));
		CHECK(Step::None == progress.current(10400 + ConnectionProgress::READY_HOLD_MS + 1).step);

		ConnectionProgress::Timings timings;
		CHECK(progress.take_completed(timings));
		CHECK(CLIENT == timings.address);
		CHECK(7000 == timings.startUpMs);
		CHECK(100 == timings.connectMs);
		CHECK(1900 == timings.uploadMs);
		CHECK(400 == timings.buildMs);
		CHECK(9400 == timings.totalMs);
		CHECK(7001 == timings.uploadedBytes);
		CHECK(!progress.take_completed(timings)); // once
	}

	void test_stored_pool_and_missing_geometry()
	{
		ConnectionProgress progress;
		process_data(progress, { 0x01 }, 0); // asks for the structure label of the stored pool
		progress.on_pool_activated(CLIENT, 300);
		auto state = progress.current(400);
		CHECK(Step::Building == state.step);
		CHECK(state.uploadSkipped);

		// Geometry values that never come do not hold the implement back for ever.
		progress.on_geometry(CLIENT, 1, 3, 500);
		CHECK(Step::Building == progress.current(300 + ConnectionProgress::GEOMETRY_WAIT_MS).step);
		CHECK(Step::Ready == progress.current(301 + ConnectionProgress::GEOMETRY_WAIT_MS).step);
	}

	void test_claims_and_losses()
	{
		ConnectionProgress progress;
		progress.on_address_claim(CLIENT, SPRAYER_NAME, 0);
		process_data(progress, { 0x00 }, 7000);
		progress.on_pool_activated(CLIENT, 7500);
		progress.on_geometry(CLIENT, 0, 0, 7600);
		CHECK(progress.is_ready(CLIENT));

		// Any device asking for the claims makes everyone claim again: a built implement stays built.
		progress.on_address_claim(CLIENT, SPRAYER_NAME, 20000);
		CHECK(progress.is_ready(CLIENT));
		CHECK(Step::None == progress.current(20000).step);

		// After it timed out, it starts up again.
		progress.on_client_lost(CLIENT);
		CHECK(Step::None == progress.current(21000).step);
		progress.on_address_claim(CLIENT, SPRAYER_NAME, 22000);
		CHECK(Step::StartingUp == progress.current(22000).step);

		// A device that never talks to the TC is given up.
		CHECK(Step::None == progress.current(22001 + ConnectionProgress::STARTUP_GIVE_UP_MS).step);
	}

	void tp_request_to_send(ConnectionProgress &progress, std::uint16_t bytes, std::uint64_t nowMs)
	{
		const std::uint8_t data[8] = { 16, static_cast<std::uint8_t>(bytes), static_cast<std::uint8_t>(bytes >> 8), 4, 0xFF, 0x00, 0xCB, 0x00 };
		progress.on_frame_to_tc(CLIENT, 0xEC00, data, 8, nowMs);
	}

	void tp_data(ConnectionProgress &progress, int packets, std::uint64_t nowMs)
	{
		const std::uint8_t data[8] = { 1, 0xC1, 0, 0, 0, 0, 0, 0 };
		for (int i = 0; i < packets; ++i) progress.on_frame_to_tc(CLIENT, 0xEB00, data, 8, nowMs);
	}

	void test_designator_changes_are_no_upload()
	{
		// As a planter does: after its pool is active it renames objects with Change Designator,
		// each sent with the transport protocol. That is no new upload.
		ConnectionProgress progress;
		process_data(progress, { 0x00 }, 1000);
		request_pool_transfer(progress, 1189, 1100);
		tp_request_to_send(progress, 1190, 1150);
		tp_data(progress, 170, 1300);
		CHECK(1190 == progress.current(1300).receivedBytes);
		progress.on_pool_activated(CLIENT, 1400);
		progress.on_geometry(CLIENT, 0, 0, 1500);
		CHECK(progress.is_ready(CLIENT));

		for (int object = 0; object < 10; ++object)
		{
			tp_request_to_send(progress, 25, 2000 + object * 100);
			tp_data(progress, 4, 2010 + object * 100);
		}
		CHECK(progress.is_ready(CLIENT));
		CHECK(Step::None == progress.current(5000).step);

		// Nor is a transport session that no request for a pool transfer announced.
		ConnectionProgress other;
		process_data(other, { 0x00 }, 0);
		const std::uint8_t data[8] = { 16, 25, 0, 4, 0xFF, 0x00, 0xCB, 0x00 };
		other.on_frame_to_tc(CLIENT, 0xEC00, data, 8, 100);
		CHECK(Step::Connecting == other.current(200).step);
	}

	void test_stalled_connection_is_given_up()
	{
		ConnectionProgress progress;
		process_data(progress, { 0x00 }, 0);
		request_pool_transfer(progress, 5000, 100);
		etp_request_to_send(progress, 5001, 150);
		etp_data(progress, 10, 200);
		CHECK(Step::Uploading == progress.current(200 + ConnectionProgress::STALL_GIVE_UP_MS).step);
		CHECK(!progress.is_ready(CLIENT));
		CHECK(Step::None == progress.current(201 + ConnectionProgress::STALL_GIVE_UP_MS).step);
		CHECK(progress.is_ready(CLIENT)); // no longer followed: it does not hold the implement back
	}

	void test_furthest_along_is_shown()
	{
		ConnectionProgress progress;
		progress.on_address_claim(130, SPRAYER_NAME, 0);
		process_data(progress, { 0x00 }, 100);
		const auto state = progress.current(200);
		CHECK(CLIENT == state.address);
		CHECK(Step::Connecting == state.step);
	}
} // namespace

int main()
{
	test_implement_names();
	test_full_connection();
	test_stored_pool_and_missing_geometry();
	test_claims_and_losses();
	test_designator_changes_are_no_upload();
	test_stalled_connection_is_given_up();
	test_furthest_along_is_shown();
	if (0 == failures)
	{
		std::printf("connection progress tests passed\n");
		return 0;
	}
	std::printf("%d connection progress check(s) failed\n", failures);
	return 1;
}
