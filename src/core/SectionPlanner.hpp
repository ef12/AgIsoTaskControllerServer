//================================================================================================
/// @file SectionPlanner.hpp
///
/// @brief TC-SC decision side: which sections should work, from where each section is going,
/// the field boundary and the ground already covered. Local field metres, no Qt or bus.
//================================================================================================
#pragma once

#include <cstdint>
#include <vector>

#include "CoverageMap.hpp"

namespace agisotc
{
	/// @brief One section on the ground: its centre, how it moves, and its width.
	struct SectionGround
	{
		GroundPoint centre;
		GroundPoint velocity; ///< Metres per second, as the section itself moved (not the implement heading).
		double widthM = 0.0;
	};

	/// @brief True when a point lies inside a closed ring (the last point may repeat the first).
	bool point_in_ring(GroundPoint point, const std::vector<GroundPoint> &ring);

	/// @brief The sections that should work now.
	/// @details A section works when it moves, and the point it reaches after lookAheadS along its
	/// own motion lies inside the boundary and was not covered before. The look-ahead follows how
	/// the section moves, not where the implement points: in a headland turn outside the field
	/// the implement soon points back at the field while its sections are still travelling
	/// outside it, and a look-ahead along the heading would switch them on there.
	/// @param[in] boundary The field boundary; with fewer than 3 points there is none and sections
	/// may work everywhere.
	std::vector<bool> wanted_section_states(const std::vector<SectionGround> &sections,
	                                        double lookAheadS,
	                                        const std::vector<GroundPoint> &boundary,
	                                        const CoverageMap &coverage,
	                                        std::uint64_t nowMs);
} // namespace agisotc
