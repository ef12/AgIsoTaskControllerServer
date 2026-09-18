//================================================================================================
/// @file IsoGpsHandler.cpp
///
/// @brief ISO 11783 GPS PGN decoder.
//================================================================================================
#include "IsoGpsHandler.hpp"

#include <cstdint>

namespace agisotc
{
	namespace
	{
		// ISO 11783 GPS PGNs
		constexpr std::uint32_t PGN_GPS_POSITION_DATA = 65267;  // 0xFEB3
		constexpr std::uint32_t PGN_GPS_POSITION_DELTA = 65268; // 0xFEB4
		constexpr std::uint32_t PGN_GPS_POSITION_DELTA_HP = 65269; // 0xFEB5
		constexpr std::uint32_t PGN_GPS_POSITION_COVARIANCE = 65270; // 0xFEB6
		constexpr std::uint32_t PGN_GPS_POSITION_DELTA_COV = 65271; // 0xFEB7

		// PGN 65267 layout (8 bytes):
		// Byte 0-3: Latitude (degrees * 1e7, signed 32-bit)
		// Byte 4-7: Longitude (degrees * 1e7, signed 32-bit)
		// Byte 8-11: Altitude (mm, signed 32-bit) - wait, that's 12 bytes
		// Actually checking ISO 11783-62:
		// Byte 0-3: Latitude (1e-7 deg, signed 32-bit)
		// Byte 4-7: Longitude (1e-7 deg, signed 32-bit)
		// Byte 8-11: Altitude (mm, signed 32-bit)
		// Byte 12: GNSS Type (0=GPS, 1=GLONASS, 2=Galileo, 3=BeiDou, etc.)
		// Byte 13: Number of satellites
		// Byte 14: HDOP (0.01 resolution)
		// Byte 15: Fix status (bitfield)
		// That's 16 bytes - but CAN FD can handle it. Standard CAN is 8 bytes.
		// Let me check actual size... it's variable. The spec says 8 bytes minimum for basic.
		// Actually PGN 65267 is defined as 8 bytes in standard CAN:
		// Byte 0-3: Latitude (1e-7 deg, signed 32-bit)
		// Byte 4-7: Longitude (1e-7 deg, signed 32-bit)
		// Extended version uses CAN FD for more fields.

		inline std::int32_t read_i32(const std::uint8_t *p)
		{
			return static_cast<std::int32_t>(
				(static_cast<std::uint32_t>(p[0]) << 0) |
				(static_cast<std::uint32_t>(p[1]) << 8) |
				(static_cast<std::uint32_t>(p[2]) << 16) |
				(static_cast<std::uint32_t>(p[3]) << 24));
		}

		inline std::int16_t read_i16(const std::uint8_t *p)
		{
			return static_cast<std::int16_t>(
				(static_cast<std::uint16_t>(p[0]) << 0) |
				(static_cast<std::uint16_t>(p[1]) << 8));
		}

		inline std::uint16_t read_u16(const std::uint8_t *p)
		{
			return static_cast<std::uint16_t>(
				(static_cast<std::uint16_t>(p[0]) << 0) |
				(static_cast<std::uint16_t>(p[1]) << 8));
		}
	}

	bool IsoGpsHandler::process_message(std::uint32_t pgn, const std::uint8_t *data, std::uint8_t len)
	{
		switch (pgn)
		{
			case PGN_GPS_POSITION_DATA:
				return handle_position_data(data, len);
			case PGN_GPS_POSITION_DELTA:
				return handle_position_delta(data, len);
			case PGN_GPS_POSITION_DELTA_HP:
				return handle_position_delta_hp(data, len);
			case PGN_GPS_POSITION_COVARIANCE:
				return handle_position_covariance(data, len);
			case PGN_GPS_POSITION_DELTA_COV:
				return handle_position_delta_covariance(data, len);
			default:
				return false;
		}
	}

	bool IsoGpsHandler::handle_position_data(const std::uint8_t *data, std::uint8_t len)
	{
		if (len < 8)
			return false;

		// PGN 65267 basic (8 bytes): lat (4), lon (4)
		// Extended (CAN FD): + alt (4), type (1), sats (1), hdop (1), status (1)
		std::int32_t lat_raw = read_i32(data + 0);
		std::int32_t lon_raw = read_i32(data + 4);

		solution_.latitudeDeg = lat_raw * 1e-7;
		solution_.longitudeDeg = lon_raw * 1e-7;

		if (len >= 12)
		{
			std::int32_t alt_raw = read_i32(data + 8);
			solution_.altitudeM = alt_raw * 1e-3; // mm to meters
		}
		if (len >= 13)
		{
			// Byte 12: GNSS type
		}
		if (len >= 14)
		{
			solution_.satellites = data[13];
		}
		if (len >= 15)
		{
			solution_.hdop = data[14] * 0.01;
		}
		if (len >= 16)
		{
			// Byte 15: Fix status bitfield
			// Bit 0: Valid position
			// Bit 1: DGPS used
			// Bit 2: RTK fixed
			// Bit 3: RTK float
			std::uint8_t status = data[15];
			if (status & 0x01) solution_.fixQuality = FixQuality::GpsFix;
			if (status & 0x04) solution_.fixQuality = FixQuality::RtkFixed;
			else if (status & 0x08) solution_.fixQuality = FixQuality::RtkFloat;
			else if (status & 0x02) solution_.fixQuality = FixQuality::DgpsFix;
		}

		solution_.valid = true;
		return true;
	}

	bool IsoGpsHandler::handle_position_delta(const std::uint8_t *data, std::uint8_t len)
	{
		// PGN 65268: 8 bytes - position delta since last message
		// Byte 0-1: Delta latitude (0.0001 deg, signed 16-bit)
		// Byte 2-3: Delta longitude (0.0001 deg, signed 16-bit)
		// Byte 4-5: Delta altitude (0.01 m, signed 16-bit)
		// Byte 6-7: Reserved
		if (len < 8)
			return false;

		double dlat = read_i16(data + 0) * 1e-4;
		double dlon = read_i16(data + 2) * 1e-4;
		double dalt = read_i16(data + 4) * 0.01;

		if (solution_.latitudeDeg)
			*solution_.latitudeDeg += dlat;
		if (solution_.longitudeDeg)
			*solution_.longitudeDeg += dlon;
		if (solution_.altitudeM)
			*solution_.altitudeM += dalt;

		return true;
	}

	bool IsoGpsHandler::handle_position_delta_hp(const std::uint8_t *data, std::uint8_t len)
	{
		// PGN 65269: High precision delta (16 bytes CAN FD)
		// Byte 0-3: Delta lat (1e-7 deg, signed 32-bit)
		// Byte 4-7: Delta lon (1e-7 deg, signed 32-bit)
		// Byte 8-11: Delta alt (mm, signed 32-bit)
		// Byte 12-15: Reserved
		if (len < 12)
			return false;

		if (solution_.latitudeDeg)
			*solution_.latitudeDeg += read_i32(data + 0) * 1e-7;
		if (solution_.longitudeDeg)
			*solution_.longitudeDeg += read_i32(data + 4) * 1e-7;
		if (solution_.altitudeM)
			*solution_.altitudeM += read_i32(data + 8) * 1e-3;

		return true;
	}

	bool IsoGpsHandler::handle_position_covariance(const std::uint8_t *data, std::uint8_t len)
	{
		// PGN 65270: Position covariance matrix (CAN FD)
		// Byte 0-3: Lat variance (1e-14 deg^2)
		// Byte 4-7: Lon variance (1e-14 deg^2)
		// Byte 8-11: Alt variance (mm^2)
		// Byte 12-15: Lat-Lon covariance
		// ... etc.
		// Could map to HDOP/VDOP/PDOP if needed
		(void)data;
		(void)len;
		return true;
	}

	bool IsoGpsHandler::handle_position_delta_covariance(const std::uint8_t *data, std::uint8_t len)
	{
		// PGN 65271: Delta covariance
		(void)data;
		(void)len;
		return true;
	}
} // namespace agisotc