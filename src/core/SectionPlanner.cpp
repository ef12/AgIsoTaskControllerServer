#include "SectionPlanner.hpp"

#include <cmath>

namespace agisotc
{
	namespace
	{
		constexpr double MIN_SECTION_SPEED_MPS = 0.3; ///< Slower than this a section applies nothing
		constexpr std::uint64_t OWN_PASS_MS = 1500; ///< Coverage younger than this is the current pass
	} // namespace

	bool point_in_ring(GroundPoint point, const std::vector<GroundPoint> &ring)
	{
		bool inside = false;
		for (std::size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++)
		{
			const auto &a = ring[i];
			const auto &b = ring[j];
			if (((a.z > point.z) != (b.z > point.z)) &&
			    (point.x < ((b.x - a.x) * (point.z - a.z) / (b.z - a.z)) + a.x))
			{
				inside = !inside;
			}
		}
		return inside;
	}

	std::vector<bool> wanted_section_states(const std::vector<SectionGround> &sections,
	                                        double lookAheadS,
	                                        const std::vector<GroundPoint> &boundary,
	                                        const CoverageMap &coverage,
	                                        std::uint64_t nowMs)
	{
		std::vector<bool> wanted(sections.size(), false);
		const bool hasBoundary = boundary.size() >= 3;
		for (std::size_t i = 0; i < sections.size(); ++i)
		{
			const auto &section = sections[i];
			const double speed = std::hypot(section.velocity.x, section.velocity.z);
			if (speed < MIN_SECTION_SPEED_MPS) continue;
			const GroundPoint ahead = { section.centre.x + (section.velocity.x * lookAheadS),
				                        section.centre.z + (section.velocity.z * lookAheadS) };
			if (hasBoundary && !point_in_ring(ahead, boundary)) continue;
			// Off when most of the width ahead was covered before, by an earlier pass.
			const double acrossX = -section.velocity.z / speed;
			const double acrossZ = section.velocity.x / speed;
			int covered = 0;
			for (const double across : { -0.3, 0.0, 0.3 })
			{
				const GroundPoint sample = { ahead.x + (acrossX * across * section.widthM), ahead.z + (acrossZ * across * section.widthM) };
				if (coverage.is_covered(sample, nowMs, OWN_PASS_MS)) ++covered;
			}
			wanted[i] = (covered < 2);
		}
		return wanted;
	}
} // namespace agisotc
