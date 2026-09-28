//================================================================================================
/// @file CoverageMap.hpp
///
/// @brief Worked area on a grid in local field metres, as section control and the coverage map
/// need it: which ground a section has already covered, and how much ground that is.
//================================================================================================
#pragma once

#include <cstdint>
#include <unordered_map>

namespace agisotc
{
	/// @brief A point in local metres on the ground plane.
	struct GroundPoint
	{
		double x = 0.0;
		double z = 0.0;
	};

	class CoverageMap
	{
	public:
		explicit CoverageMap(double cellSizeM = 0.25);

		/// @brief Marks the ground a section swept while its centre moved from one point to the next.
		/// @param[in] from Section centre at the previous step.
		/// @param[in] to Section centre now.
		/// @param[in] widthM Section width, across the direction of travel.
		/// @param[in] nowMs Time of the pass, kept per cell.
		/// @returns The ground this pass covered for the first time, in square metres.
		double cover_swath(GroundPoint from, GroundPoint to, double widthM, std::uint64_t nowMs);

		/// @brief True when the ground at a point was covered at least minAgeMs ago. The age keeps a
		/// section from switching itself off on the strip it is covering right now.
		bool is_covered(GroundPoint point, std::uint64_t nowMs, std::uint64_t minAgeMs) const;

		double covered_area_m2() const;
		void clear();

	private:
		std::int64_t key(double x, double z) const;

		double cellSize;
		std::unordered_map<std::int64_t, std::uint64_t> coveredAtMs; ///< First time each cell was covered.
	};
} // namespace agisotc
