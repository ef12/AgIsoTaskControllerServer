#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "isobus::Utility" for configuration "MinSizeRel"
set_property(TARGET isobus::Utility APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(isobus::Utility PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "CXX"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/Utility.lib"
  )

list(APPEND _cmake_import_check_targets isobus::Utility )
list(APPEND _cmake_import_check_files_for_isobus::Utility "${_IMPORT_PREFIX}/lib/Utility.lib" )

# Import target "isobus::Isobus" for configuration "MinSizeRel"
set_property(TARGET isobus::Isobus APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(isobus::Isobus PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "CXX"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/Isobus.lib"
  )

list(APPEND _cmake_import_check_targets isobus::Isobus )
list(APPEND _cmake_import_check_files_for_isobus::Isobus "${_IMPORT_PREFIX}/lib/Isobus.lib" )

# Import target "isobus::HardwareIntegration" for configuration "MinSizeRel"
set_property(TARGET isobus::HardwareIntegration APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(isobus::HardwareIntegration PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "CXX"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/HardwareIntegration.lib"
  )

list(APPEND _cmake_import_check_targets isobus::HardwareIntegration )
list(APPEND _cmake_import_check_files_for_isobus::HardwareIntegration "${_IMPORT_PREFIX}/lib/HardwareIntegration.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
