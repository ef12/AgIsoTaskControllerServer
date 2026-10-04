//================================================================================================
/// @file IsoXmlTaskData.hpp
///
/// @brief Reads the prescriptions of an ISO 11783-10 data transfer file set (TASKDATA.XML and its
/// grid files): the tasks with their treatment zones, polygons and grid, and the partfield
/// boundaries they refer to. No Qt dependency.
//================================================================================================
#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "PrescriptionMap.hpp"

namespace agisotc
{
	/// @brief An XML element with its attributes and child elements. ISOXML carries everything in
	/// attributes, so text content is not kept.
	struct XmlElement
	{
		std::string name;
		std::vector<std::pair<std::string, std::string>> attributes;
		std::vector<XmlElement> children;

		/// @brief The value of an attribute, or nullptr.
		const std::string *attribute(std::string_view key) const;
	};

	/// @brief Parses an XML document into its root element.
	/// @returns False with a message when the document is not well formed.
	bool parse_xml(std::string_view text, XmlElement &root, std::string &error);

	/// @brief A partfield (PFD) with its boundary.
	struct ImportedField
	{
		std::string id;
		std::string name;
		std::vector<GeoPoint> boundary; ///< The exterior ring of its boundary polygon, empty if none.
	};

	/// @brief A task (TSK) with its prescription.
	struct ImportedTask
	{
		std::string id;
		std::string name;
		std::string partfieldId;
		Prescription prescription;
	};

	struct TaskDataImport
	{
		std::vector<ImportedField> fields;
		std::vector<ImportedTask> tasks;
		std::vector<std::string> warnings;
	};

	/// @brief Reads a file of the transfer set by its name (for example "GRD00001.bin"), or nothing.
	using TaskDataFileReader = std::function<std::optional<std::vector<std::uint8_t>>(const std::string &fileName)>;

	/// @brief Reads the tasks and partfields of a TASKDATA.XML. Grid files and external XML files
	/// (XFR) come through the reader.
	/// @returns False with a message when the file is no ISO 11783 task data.
	bool import_task_data(std::string_view xml, const TaskDataFileReader &readFile, TaskDataImport &result, std::string &error);
} // namespace agisotc
