#include "PrescriptionImage.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace agisotc
{
	void PrescriptionImageSlot::set(const QImage &image)
	{
		std::lock_guard<std::mutex> lock(mutex);
		current = image;
	}

	QImage PrescriptionImageSlot::get()
	{
		std::lock_guard<std::mutex> lock(mutex);
		return current;
	}

	PrescriptionImageProvider::PrescriptionImageProvider(std::shared_ptr<PrescriptionImageSlot> imageSlot) :
	  QQuickImageProvider(QQuickImageProvider::Image),
	  slot(std::move(imageSlot))
	{
	}

	QImage PrescriptionImageProvider::requestImage(const QString &, QSize *size, const QSize &)
	{
		QImage image = (nullptr != slot) ? slot->get() : QImage();
		if (image.isNull())
		{
			image = QImage(1, 1, QImage::Format_ARGB32);
			image.fill(Qt::transparent);
		}
		if (nullptr != size) *size = image.size();
		return image;
	}

	QColor rate_colour(double position)
	{
		// A diverging red - yellow - green scale, as farm management software shows rate maps.
		static constexpr std::array<std::array<int, 3>, 5> STOPS = { {
		  { 215, 25, 28 },
		  { 253, 174, 97 },
		  { 255, 255, 191 },
		  { 166, 217, 106 },
		  { 26, 150, 65 },
		} };
		const double scaled = std::clamp(std::isfinite(position) ? position : 0.5, 0.0, 1.0) * (STOPS.size() - 1);
		const auto lower = static_cast<std::size_t>(std::floor(scaled));
		const auto upper = std::min(lower + 1, STOPS.size() - 1);
		const double fraction = scaled - lower;
		auto mix = [&](std::size_t channel) {
			return static_cast<int>(std::lround(STOPS[lower][channel] + ((STOPS[upper][channel] - STOPS[lower][channel]) * fraction)));
		};
		return QColor(mix(0), mix(1), mix(2));
	}
} // namespace agisotc
