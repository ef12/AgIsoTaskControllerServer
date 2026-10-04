//================================================================================================
/// @file PrescriptionJson.hpp
///
/// @brief A task's prescription in the task file (JSON): its zones, polygons and grid, the grid
/// cells base64-coded as in the ISOXML grid file.
//================================================================================================
#pragma once

#include <QJsonObject>

#include "PrescriptionMap.hpp"

namespace agisotc
{
	QJsonObject prescription_to_json(const Prescription &prescription);
	Prescription prescription_from_json(const QJsonObject &object);
} // namespace agisotc
