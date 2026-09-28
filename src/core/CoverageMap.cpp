#include "CoverageMap.hpp"

#include <algorithm>
#include <cmath>

namespace agisotc
{
	CoverageMap::CoverageMap(double cellSizeM) :
	  cellSize(std::max(0.05, cellSizeM))
	{
	}

	std::int64_t CoverageMap::key(double x, double z) const
	{
		const auto column = static_cast<std::int64_t>(std::floor(x / cellSize));
		const auto row = static_cast<std::int64_t>(std::floor(z / cellSize));
		return (column << 32) ^ (row & 0xFFFFFFFFLL);
	}

	double CoverageMap::cover_swath(GroundPoint from, GroundPoint to, double widthM, std::uint64_t nowMs)
	{
		const double dx = to.x - from.x;
		const double dz = to.z - from.z;
		const double length = std::hypot(dx, dz);
		if ((length <= 1e-6) || (widthM <= 0.0)) return 0.0;

		// Unit vectors along and across the movement.
		const double alongX = dx / length;
		const double alongZ = dz / length;
		const double acrossX = -alongZ;
		const double acrossZ = alongX;

		// Sample the swept rectangle at half a cell, so no cell inside it is skipped.
		const double step = cellSize / 2.0;
		const int alongSteps = std::max(1, static_cast<int>(std::ceil(length / step)));
		const int acrossSteps = std::max(1, static_cast<int>(std::ceil(widthM / step)));
		std::size_t newCells = 0;
		for (int a = 0; a <= alongSteps; ++a)
		{
			const double along = length * static_cast<double>(a) / alongSteps;
			for (int c = 0; c <= acrossSteps; ++c)
			{
				const double across = widthM * ((static_cast<double>(c) / acrossSteps) - 0.5);
				const double x = from.x + (alongX * along) + (acrossX * across);
				const double z = from.z + (alongZ * along) + (acrossZ * across);
				if (coveredAtMs.emplace(key(x, z), nowMs).second) ++newCells;
			}
		}
		return static_cast<double>(newCells) * cellSize * cellSize;
	}

	bool CoverageMap::is_covered(GroundPoint point, std::uint64_t nowMs, std::uint64_t minAgeMs) const
	{
		const auto cell = coveredAtMs.find(key(point.x, point.z));
		return (cell != coveredAtMs.end()) && (nowMs >= cell->second) && ((nowMs - cell->second) >= minAgeMs);
	}

	double CoverageMap::covered_area_m2() const
	{
		return static_cast<double>(coveredAtMs.size()) * cellSize * cellSize;
	}

	void CoverageMap::clear()
	{
		coveredAtMs.clear();
	}
} // namespace agisotc
