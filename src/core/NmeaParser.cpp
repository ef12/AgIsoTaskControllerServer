//================================================================================================
/// @file NmeaParser.cpp
///
/// @brief NMEA 0183 parser implementation.
//================================================================================================
#include "NmeaParser.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>

namespace agisotc
{
	bool NmeaParser::parse_sentence(std::string_view sentence)
	{
		if (sentence.empty() || sentence[0] != '$')
			return false;

		if (!verify_checksum(sentence))
			return false;

		// Strip $ and *checksum
		auto star = sentence.rfind('*');
		std::string_view body = sentence.substr(1, star == std::string_view::npos ? std::string_view::npos : star - 1);

		auto fields = split_fields(body);
		if (fields.size() < 2)
			return false;

		std::string_view talker = fields[0].substr(0, 2); // GP, GN, GL, GA, GB
		std::string_view type = fields[0].substr(2);      // GGA, RMC, etc.

		// We accept any talker ID for these sentence types
		if (type == "GGA")
			return handle_gga(body);
		if (type == "RMC")
			return handle_rmc(body);
		if (type == "GSA")
			return handle_gsa(body);
		if (type == "GSV")
			return handle_gsv(body);
		if (type == "VTG")
			return handle_vtg(body);
		if (type == "GLL")
			return handle_gll(body);

		return false;
	}

	void NmeaParser::reset()
	{
		solution.clear();
		status = {};
		gsv_buffer.clear();
	}

	std::vector<std::string_view> NmeaParser::split_fields(std::string_view body)
	{
		std::vector<std::string_view> fields;
		std::size_t start = 0;
		while (start <= body.size())
		{
			auto end = body.find(',', start);
			fields.emplace_back(body.substr(start, end - start));
			if (end == std::string_view::npos)
				break;
			start = end + 1;
		}
		return fields;
	}

	std::optional<double> NmeaParser::parse_degrees_minutes(std::string_view dm, std::string_view hemi)
	{
		if (dm.empty() || hemi.empty())
			return std::nullopt;

		double raw = 0.0;
		auto [ptr, ec] = std::from_chars(dm.data(), dm.data() + dm.size(), raw);
		if (ec != std::errc())
			return std::nullopt;

		// ddmm.mmmm or dddmm.mmmm
		double degrees = std::floor(raw / 100.0);
		double minutes = raw - degrees * 100.0;
		double decimal = degrees + minutes / 60.0;

		char h = hemi[0];
		if (h == 'S' || h == 'W')
			decimal = -decimal;

		return decimal;
	}

	std::optional<double> NmeaParser::parse_double(std::string_view s)
	{
		if (s.empty())
			return std::nullopt;
		double v = 0.0;
		auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
		if (ec != std::errc())
			return std::nullopt;
		return v;
	}

	std::optional<std::uint32_t> NmeaParser::parse_uint(std::string_view s)
	{
		if (s.empty())
			return std::nullopt;
		std::uint32_t v = 0;
		auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
		if (ec != std::errc())
			return std::nullopt;
		return v;
	}

	std::optional<std::int32_t> NmeaParser::parse_int(std::string_view s)
	{
		if (s.empty())
			return std::nullopt;
		std::int32_t v = 0;
		auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
		if (ec != std::errc())
			return std::nullopt;
		return v;
	}

	bool NmeaParser::verify_checksum(std::string_view sentence)
	{
		auto star = sentence.rfind('*');
		if (star == std::string_view::npos || star + 3 > sentence.size())
			return false;

		std::uint8_t expected = 0;
		for (std::size_t i = 1; i < star; ++i) // skip '$'
			expected ^= static_cast<std::uint8_t>(sentence[i]);

		std::string_view hex = sentence.substr(star + 1, 2);
		std::uint8_t got = 0;
		auto [ptr, ec] = std::from_chars(hex.data(), hex.data() + hex.size(), got, 16);
		return ec == std::errc() && got == expected;
	}

	bool NmeaParser::handle_gga(std::string_view body)
	{
		// $--GGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
		//  0    1        2  3       4  5 6  7  8  9  10 11 12  13  14
		auto fields = split_fields(body);
		if (fields.size() < 15)
			return false;

		solution.timestampMs = 0; // No date in GGA, would need RMC/ZDA

		if (auto lat = parse_degrees_minutes(fields[2], fields[3]); lat)
			solution.latitudeDeg = *lat;
		if (auto lon = parse_degrees_minutes(fields[4], fields[5]); lon)
			solution.longitudeDeg = *lon;
		if (auto q = parse_uint(fields[6]); q && *q <= 8)
			solution.fixQuality = static_cast<FixQuality>(*q);
		if (auto sats = parse_uint(fields[7]); sats)
			solution.satellites = static_cast<std::uint8_t>(*sats);
		if (auto h = parse_double(fields[8]); h)
			solution.hdop = *h;
		if (auto alt = parse_double(fields[9]); alt)
			solution.altitudeM = *alt;
		if (auto hmsl = parse_double(fields[11]); hmsl)
			solution.heightMslM = *hmsl;
		if (auto age = parse_double(fields[13]); age)
			solution.ageOfDgpsS = *age;
		if (auto sid = parse_uint(fields[14]); sid)
			solution.dgpsStationId = static_cast<std::uint16_t>(*sid);

		solution.valid = solution.latitudeDeg && solution.longitudeDeg &&
		                 solution.fixQuality && *solution.fixQuality != FixQuality::Invalid;
		return true;
	}

	bool NmeaParser::handle_rmc(std::string_view body)
	{
		// $--RMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a*hh
		//  0    1        2 3       4  5       6  7  8      9  10 11
		auto fields = split_fields(body);
		if (fields.size() < 12)
			return false;

		if (fields[2] != "A") // Status: A=active, V=void
		{
			solution.valid = false;
			return true;
		}

		// Time + date
		if (auto t = parse_double(fields[1]); t)
		{
			// hhmmss.ss
			std::uint32_t hhmmss = static_cast<std::uint32_t>(*t);
			std::uint32_t hh = hhmmss / 10000;
			std::uint32_t mm = (hhmmss % 10000) / 100;
			std::uint32_t ss = hhmmss % 100;
			// Note: no date in solution yet, would need to combine with field[9] (ddmmyy)
			// For now just store a relative timestamp
			solution.timestampMs = ((hh * 60 + mm) * 60 + ss) * 1000;
		}

		if (auto lat = parse_degrees_minutes(fields[3], fields[4]); lat)
			solution.latitudeDeg = *lat;
		if (auto lon = parse_degrees_minutes(fields[5], fields[6]); lon)
			solution.longitudeDeg = *lon;
		if (auto spd = parse_double(fields[7]); spd)
			solution.speedMps = *spd * 0.514444; // knots to m/s
		if (auto crs = parse_double(fields[8]); crs)
			solution.courseDeg = *crs;

		solution.valid = solution.latitudeDeg && solution.longitudeDeg;
		return true;
	}

	bool NmeaParser::handle_gsa(std::string_view body)
	{
		// $--GSA,A,x,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,x.x,x.x,x.x*hh
		auto fields = split_fields(body);
		if (fields.size() < 18)
			return false;

		status.satellites.clear();
		status.satellitesUsed = 0;

		// Fields 3-14 are PRNs used in fix
		for (std::size_t i = 3; i <= 14 && i < fields.size(); ++i)
		{
			if (auto prn = parse_uint(fields[i]); prn && *prn > 0)
			{
				SatelliteInfo sat;
				sat.prn = static_cast<std::uint8_t>(*prn);
				sat.usedInFix = true;
				status.satellites.push_back(sat);
				++status.satellitesUsed;
			}
		}
		if (auto pdop = parse_double(fields[15]); pdop)
			solution.pdop = *pdop;
		if (auto hdop = parse_double(fields[16]); hdop)
			solution.hdop = *hdop;
		if (auto vdop = parse_double(fields[17]); vdop)
			solution.vdop = *vdop;

		return true;
	}

	bool NmeaParser::handle_gsv(std::string_view body)
	{
		// $--GSV,x,x,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx*hh
		auto fields = split_fields(body);
		if (fields.size() < 4)
			return false;

		if (auto totalMsg = parse_uint(fields[1]); totalMsg)
		{
			if (auto msgNum = parse_uint(fields[2]); msgNum)
			{
				if (*msgNum == 1)
					gsv_buffer.clear();
				gsv_buffer.push_back(body);
				if (*msgNum == *totalMsg)
				{
					// Process complete GSV set
					status.satellitesInView = 0;
					for (const auto &msg : gsv_buffer)
					{
						auto f = split_fields(msg);
						if (f.size() < 4)
							continue;
						if (auto satsInView = parse_uint(f[3]); satsInView)
							status.satellitesInView = *satsInView;
						// Each satellite block is 4 fields: PRN, elevation, azimuth, SNR
						for (std::size_t i = 4; i + 3 < f.size(); i += 4)
						{
							SatelliteInfo sat;
							if (auto prn = parse_uint(f[i]); prn) sat.prn = static_cast<std::uint8_t>(*prn);
							if (auto el = parse_uint(f[i + 1]); el) sat.elevationDeg = static_cast<std::uint8_t>(*el);
							if (auto az = parse_uint(f[i + 2]); az) sat.azimuthDeg = static_cast<std::uint16_t>(*az);
							if (auto snr = parse_uint(f[i + 3]); snr) sat.snrDbHz = static_cast<std::uint8_t>(*snr);
							status.satellites.push_back(sat);
						}
					}
					gsv_buffer.clear();
				}
			}
		}
		return true;
	}

	bool NmeaParser::handle_vtg(std::string_view body)
	{
		// $--VTG,x.x,T,x.x,M,x.x,N,x.x,K,a*hh
		auto fields = split_fields(body);
		if (fields.size() < 9)
			return false;

		if (auto crs = parse_double(fields[1]); crs)
			solution.courseDeg = *crs;
		if (auto spd = parse_double(fields[7]); spd)
			solution.speedMps = *spd / 3.6; // km/h to m/s

		return true;
	}

	bool NmeaParser::handle_gll(std::string_view body)
	{
		// $--GLL,llll.ll,a,yyyyy.yy,a,hhmmss.ss,A,a*hh
		auto fields = split_fields(body);
		if (fields.size() < 7)
			return false;

		if (fields[6] != "A")
		{
			solution.valid = false;
			return true;
		}

		if (auto lat = parse_degrees_minutes(fields[1], fields[2]); lat)
			solution.latitudeDeg = *lat;
		if (auto lon = parse_degrees_minutes(fields[3], fields[4]); lon)
			solution.longitudeDeg = *lon;
		if (auto t = parse_double(fields[5]); t)
		{
			std::uint32_t hhmmss = static_cast<std::uint32_t>(*t);
			std::uint32_t hh = hhmmss / 10000;
			std::uint32_t mm = (hhmmss % 10000) / 100;
			std::uint32_t ss = hhmmss % 100;
			solution.timestampMs = ((hh * 60 + mm) * 60 + ss) * 1000;
		}

		solution.valid = solution.latitudeDeg && solution.longitudeDeg;
		return true;
	}
} // namespace agisotc