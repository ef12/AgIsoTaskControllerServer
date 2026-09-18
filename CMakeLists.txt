cmake_minimum_required(VERSION 3.21)

project(
  "AgIsoTaskControllerServer"
  LANGUAGES CXX
  VERSION 0.1.0)

option(AGISOTC_BUILD_GUI "Build the Qt Quick GUI application" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

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
    isobus::HardwareIntegration)

if(AGISOTC_BUILD_GUI)
  find_package(Qt6 6.5 REQUIRED COMPONENTS Core Quick Qml)
  qt_standard_project_setup()

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
      Qt6::Qml)

  if(WIN32)
    set_target_properties(AgIsoTaskControllerServer PROPERTIES WIN32_EXECUTABLE TRUE)
  endif()
endif()
