//================================================================================================
/// @file IsoGpsHandler.hpp
///
/// @brief ISO 11783 GPS PGN handler (PGN 65267 Position, 65268 Delta, etc.)
/// Uses AgIsoStack++ PGN definitions.
//================================================================================================
#pragma once

#include "GpsTypes.hpp"

#include <cstdint>
#include <optional>

namespace agisotc
{
	/// @brief Handles ISO 11783 GPS PGNs from the CAN bus.
	/// PGN 65267 (0xFEB3) - GPS Position Data
	/// PGN 65268 (0xFEB4) - GPS Position Delta
	/// PGN 65269 (0xFEB5) - GPS Position Delta High Precision
	/// PGN 65270 (0xFEB6) - GPS Position Covariance
	/// PGN 65271 (0xFEB7) - GPS Position Delta Covariance
	class IsoGpsHandler
	{
	public:
		/// @brief Processes a CAN message. Returns true if it was a GPS PGN we handled.
		bool process_message(std::uint32_t pgn, const std::uint8_t *data, std::uint8_t len);

		/// @brief Gets the latest parsed GPS solution.
		const GpsSolution &solution() const { return solution_; }

		void reset() { solution_.clear(); }

	private:
		bool handle_position_data(const std::uint8_t *data, std::uint8_t len);          // PGN 65267
		bool handle_position_delta(const std::uint8_t *data, std::uint8_t len);         // PGN 65268
		bool handle_position_delta_hp(const std::uint8_t *data, std::uint8_t len);      // PGN 65269
		bool handle_position_covariance(const std::uint8_t *data, std::uint8_t len);    // PGN 65270
		bool handle_position_delta_covariance(const std::uint8_t *data, std::uint8_t len); // PGN 65271

		GpsSolution solution_;
	};
} // namespace agisotc