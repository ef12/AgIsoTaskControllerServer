cmake_minimum_required(VERSION 3.21)

project(
  "AgIsoTaskControllerServer"
  LANGUAGES CXX
  VERSION 0.1.0)

option(AGISOTC_BUILD_GUI "Build the Qt Quick GUI application" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  CAN_STACK
  GIT_REPOSITORY https://github.com/ef12/AgIsoStack-plus-plus.git
  GIT_TAG 1eb0a89f21e0c2ea57a2c63b93a7d3b2218bc66f)
FetchContent_MakeAvailable(CAN_STACK)

# Core library: TC server wrapper + CAN setup. No Qt dependency, so it can be
# built and unit-tested anywhere the stack builds.
add_library(tcserver_core STATIC
  src/core/TcServerCore.cpp
  src/core/CanBusManager.cpp)
target_include_directories(tcserver_core PUBLIC src/core)
target_link_libraries(tcserver_core
  PUBLIC
    isobus::Isobus
    isobus::HardwareIntegration
    isobus::Utility)

if(AGISOTC_BUILD_GUI)
  find_package(Qt6 6.5 REQUIRED COMPONENTS Core Quick Quick3D Qml)
  qt_standard_project_setup()
  set(CMAKE_AUTORCC ON)

  qt_add_executable(AgIsoTaskControllerServer
    src/gui/main.cpp
    src/gui/TcBridge.cpp
    src/gui/TcModels.cpp
    resources.qrc)

  target_link_libraries(AgIsoTaskControllerServer
    PRIVATE
      tcserver_core
      Qt6::Core
      Qt6::Quick
      Qt6::Quick3D
      Qt6::Qml)

  if(WIN32)
    set_target_properties(AgIsoTaskControllerServer PROPERTIES WIN32_EXECUTABLE TRUE)
    if(MSVC)
      # WCAN is a vendored C static library built with cl's static-runtime
      # default. Resolve its CRT references against the Qt application's
      # dynamic runtime to avoid linking two CRT implementations.
      target_link_options(AgIsoTaskControllerServer PRIVATE /NODEFAULTLIB:LIBCMT)
    endif()
  endif()

  # Install the executable and QML resources
  install(TARGETS AgIsoTaskControllerServer
    RUNTIME DESTINATION bin
    COMPONENT applications)
  install(FILES qml/Main.qml qml/TopBar.qml qml/ClientsPanel.qml qml/DdopPanel.qml
    qml/ProcessDataPanel.qml qml/CommandPanel.qml qml/SectionView3D.qml qml/LogPanel.qml
    DESTINATION qml
    COMPONENT applications)
endif()

# CPack packaging (NSIS on Windows, DMG on macOS, DEB on Linux)
set(CPACK_PACKAGE_NAME "AgIsoTaskControllerServer")
set(CPACK_PACKAGE_VENDOR "Open-Agriculture")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "An ISO11783-10 Task Controller Server")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "AgIsoTaskControllerServer")
set(CPACK_COMPONENTS_ALL applications)
set(CPACK_COMPONENT_APPLICATIONS_DISPLAY_NAME "AgIsoTaskControllerServer")
set(CPACK_COMPONENT_APPLICATIONS_DESCRIPTION "A free and open source ISO11783-10 Task Controller Server")
set(CPACK_PACKAGE_CONTACT "delgrossoengineering@protonmail.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_EXECUTABLES "AgIsoTaskControllerServer" "AgIsoTaskControllerServer")
set(CPACK_ALL_INSTALL_TYPES Full)
set(CPACK_COMPONENT_LIBRARIES_INSTALL_TYPES Full)
set(CPACK_COMPONENT_APPLICATIONS_INSTALL_TYPES Full)

# Windows-only NSIS installer
if(WIN32)
  set(CPACK_GENERATOR "NSIS")
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/doc/logo.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "${CMAKE_CURRENT_SOURCE_DIR}/doc/logo.ico")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/doc/logo.ico")
    set(CPACK_NSIS_MUI_UNICON "${CMAKE_CURRENT_SOURCE_DIR}/doc/logo.ico")
  endif()
  set(CPACK_NSIS_CONTACT ${CPACK_PACKAGE_CONTACT})
  set(CPACK_NSIS_DISPLAY_NAME "AgIsoTaskControllerServer")
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
else()
  set(CPACK_GENERATOR "ZIP")
endif()

# Only package the 'applications' component (app + QML + runtime), exclude default component (stack)
set(CPACK_COMPONENTS_ALL "applications")
set(CPACK_COMPONENT_DEFAULT_INSTALL_TYPES "")

# Only install the application target and QML files - not the transitive stack dependencies
set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT applications)
include(InstallRequiredSystemLibraries)
include(CPack)
