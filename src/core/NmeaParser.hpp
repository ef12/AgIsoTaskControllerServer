//================================================================================================
/// @file NmeaParser.hpp
///
/// @brief Lightweight NMEA 0183 sentence parser (GGA, RMC, GSA, GSV, VTG, GLL).
//================================================================================================
#pragma once

#include "GpsTypes.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace agisotc
{
	class NmeaParser
	{
	public:
		/// @brief Parses a complete NMEA sentence (including $... and *checksum).
		/// @returns true if sentence was recognized and parsed, false otherwise.
		bool parse_sentence(std::string_view sentence);

		/// @brief Gets the last parsed position solution.
		const GpsSolution &last_solution() const { return solution; }

		/// @brief Gets the last parsed GNSS status.
		const GnssStatus &last_status() const { return status; }

		/// @brief Clears internal state.
		void reset();

	private:
		// Sentence handlers
		bool handle_gga(std::string_view body);
		bool handle_rmc(std::string_view body);
		bool handle_gsa(std::string_view body);
		bool handle_gsv(std::string_view body);
		bool handle_vtg(std::string_view body);
		bool handle_gll(std::string_view body);

		// Helpers
		static std::vector<std::string_view> split_fields(std::string_view body);
		static std::optional<double> parse_degrees_minutes(std::string_view dm, std::string_view hemi);
		static std::optional<double> parse_double(std::string_view s);
		static std::optional<std::uint32_t> parse_uint(std::string_view s);
		static std::optional<std::int32_t> parse_int(std::string_view s);
		static bool verify_checksum(std::string_view sentence);

		GpsSolution solution;
		GnssStatus status;
		std::vector<std::string_view> gsv_buffer; ///< Accumulates multi-part GSV
	};
} // namespace agisotc