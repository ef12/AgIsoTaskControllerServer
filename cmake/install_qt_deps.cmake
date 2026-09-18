#================================================================================================
/// @file install_qt_deps.cmake
///
/// @brief CMake functions to install Qt dependencies (called from install(CODE)).
//================================================================================================

function(install_qt_deps BUILD_DIR)
  message(STATUS "Installing Qt dependencies from: ${BUILD_DIR}")

  # List of Qt DLLs to install
  set(QT_DLLS
    Qt6Core.dll
    Qt6Gui.dll
    Qt6Network.dll
    Qt6OpenGL.dll
    Qt6Qml.dll
    Qt6QmlModels.dll
    Qt6Quick.dll
    Qt6Quick3DUtils.dll
    Qt6Svg.dll
    D3Dcompiler_47.dll
  )

  # Install Qt DLLs
  foreach(dll ${QT_DLLS})
    if(EXISTS "${BUILD_DIR}/${dll}")
      file(INSTALL DESTINATION "bin" TYPE FILE FILES "${BUILD_DIR}/${dll}")
    else()
      message(WARNING "Qt DLL not found: ${BUILD_DIR}/${dll}")
    endif()
  endforeach()

  # Install tls plugins
  if(EXISTS "${BUILD_DIR}/tls")
    file(GLOB TLS_DLLS "${BUILD_DIR}/tls/*.dll")
    foreach(dll ${TLS_DLLS})
      get_filename_component(name ${dll} NAME)
      file(INSTALL DESTINATION "bin/tls" TYPE FILE FILES "${dll}")
    endforeach()
  endif()

  # Install generic plugins
  if(EXISTS "${BUILD_DIR}/generic")
    file(GLOB GENERIC_DLLS "${BUILD_DIR}/generic/*.dll")
    foreach(dll ${GENERIC_DLLS})
      get_filename_component(name ${dll} NAME)
      file(INSTALL DESTINATION "bin/generic" TYPE FILE FILES "${dll}")
    endforeach()
  endif()
endfunction()

# If called directly (not via install(CODE)), parse arguments and call function
if(CMAKE_CURRENT_SCRIPT_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
  if(CMAKE_ARGC GREATER 1)
    install_qt_deps(${CMAKE_ARGV1})
  else()
    message(FATAL_ERROR "Usage: cmake -P install_qt_deps.cmake <build_dir>")
  endif()
endif()