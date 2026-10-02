#include "GridGeometry.hpp"

#include <QByteArray>
#include <QVector3D>

#include <algorithm>
#include <cmath>

namespace agisotc
{
	namespace
	{
		/// Lines per direction at most, so a tiny spacing cannot build a huge mesh.
		constexpr int MaxLinesPerAxis = 2001;
	}

	GridGeometry::GridGeometry(QQuick3DObject *parent) :
	  QQuick3DGeometry(parent)
	{
		rebuild();
	}

	double GridGeometry::spacing() const
	{
		return gridSpacing;
	}

	void GridGeometry::setSpacing(double metres)
	{
		if ((metres > 0.0) && (metres != gridSpacing))
		{
			gridSpacing = metres;
			rebuild();
			emit spacingChanged();
		}
	}

	double GridGeometry::extent() const
	{
		return gridExtent;
	}

	void GridGeometry::setExtent(double metres)
	{
		if ((metres > 0.0) && (metres != gridExtent))
		{
			gridExtent = metres;
			rebuild();
			emit extentChanged();
		}
	}

	void GridGeometry::rebuild()
	{
		// Lines on the multiples of the spacing, so the grid stays on the world origin.
		const int half = std::min(static_cast<int>(std::floor(gridExtent / gridSpacing)), MaxLinesPerAxis / 2);
		const float reach = static_cast<float>(half * gridSpacing);
		QByteArray vertices;
		vertices.resize(static_cast<qsizetype>((2 * half + 1) * 4 * 3 * sizeof(float)));
		auto *out = reinterpret_cast<float *>(vertices.data());
		for (int i = -half; i <= half; ++i)
		{
			const float at = static_cast<float>(i * gridSpacing);
			// a line along Z at x = at, and one along X at z = at
			*out++ = at;     *out++ = 0.0f; *out++ = -reach;
			*out++ = at;     *out++ = 0.0f; *out++ = reach;
			*out++ = -reach; *out++ = 0.0f; *out++ = at;
			*out++ = reach;  *out++ = 0.0f; *out++ = at;
		}

		clear();
		setVertexData(vertices);
		setStride(3 * sizeof(float));
		setPrimitiveType(QQuick3DGeometry::PrimitiveType::Lines);
		addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0, QQuick3DGeometry::Attribute::F32Type);
		setBounds(QVector3D(-reach, 0.0f, -reach), QVector3D(reach, 0.0f, reach));
		update();
	}
} // namespace agisotc
