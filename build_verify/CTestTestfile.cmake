# CMake generated Testfile for 
# Source directory: C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer
# Build directory: C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/build_verify
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test("core_workflow" "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/build_verify/Debug/tcserver_core_tests.exe")
  set_tests_properties("core_workflow" PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/CMakeLists.txt;40;add_test;C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test("core_workflow" "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/build_verify/Release/tcserver_core_tests.exe")
  set_tests_properties("core_workflow" PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/CMakeLists.txt;40;add_test;C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test("core_workflow" "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/build_verify/MinSizeRel/tcserver_core_tests.exe")
  set_tests_properties("core_workflow" PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/CMakeLists.txt;40;add_test;C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test("core_workflow" "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/build_verify/RelWithDebInfo/tcserver_core_tests.exe")
  set_tests_properties("core_workflow" PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/CMakeLists.txt;40;add_test;C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/CMakeLists.txt;0;")
else()
  add_test("core_workflow" NOT_AVAILABLE)
endif()
subdirs("_deps/can_stack-build")
