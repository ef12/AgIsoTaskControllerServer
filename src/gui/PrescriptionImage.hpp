//================================================================================================
/// @file PrescriptionImage.hpp
///
/// @brief The prescription map as an image for the views ("image://prescription/<serial>"): the
/// bridge renders a layer into the slot, the QML image provider hands it out.
//================================================================================================
#pragma once

#include <memory>
#include <mutex>

#include <QColor>
#include <QImage>
#include <QQuickImageProvider>

namespace agisotc
{
	/// @brief The image last rendered, shared between the bridge (GUI thread) and the provider
	/// (the QML engine's image loading thread).
	class PrescriptionImageSlot
	{
	public:
		void set(const QImage &image);
		QImage get();

	private:
		std::mutex mutex;
		QImage current;
	};

	class PrescriptionImageProvider : public QQuickImageProvider
	{
	public:
		explicit PrescriptionImageProvider(std::shared_ptr<PrescriptionImageSlot> slot);
		QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

	private:
		std::shared_ptr<PrescriptionImageSlot> slot;
	};

	/// @brief The colour of a rate on the map's scale: low red, middle yellow, high green.
	/// @param[in] position 0 for the lowest rate of the layer, 1 for the highest.
	QColor rate_colour(double position);
} // namespace agisotc
