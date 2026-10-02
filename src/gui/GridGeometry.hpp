//================================================================================================
/// @file GridGeometry.hpp
///
/// @brief A ground grid for the 3D view: lines along X and Z every spacing metres, out to extent
/// metres from the origin in every direction, as one line-list mesh (one draw call, lines one
/// pixel wide at any distance, unlike a tiled texture that blurs when seen at a flat angle).
//================================================================================================
#pragma once

#include <QtQuick3D/QQuick3DGeometry>

namespace agisotc
{
	class GridGeometry : public QQuick3DGeometry
	{
		Q_OBJECT
		Q_PROPERTY(double spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
		Q_PROPERTY(double extent READ extent WRITE setExtent NOTIFY extentChanged)

	public:
		explicit GridGeometry(QQuick3DObject *parent = nullptr);

		double spacing() const;
		void setSpacing(double metres);
		double extent() const;
		void setExtent(double metres);

	signals:
		void spacingChanged();
		void extentChanged();

	private:
		void rebuild();

		double gridSpacing = 10.0;
		double gridExtent = 1000.0;
	};
} // namespace agisotc
