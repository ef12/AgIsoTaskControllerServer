# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/build_verify/_deps/can_stack-src")
  file(MAKE_DIRECTORY "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/build_verify/_deps/can_stack-src")
endif()
file(MAKE_DIRECTORY
  "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/build_verify/_deps/can_stack-build"
  "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/build_verify/_deps/can_stack-subbuild/can_stack-populate-prefix"
  "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/build_verify/_deps/can_stack-subbuild/can_stack-populate-prefix/tmp"
  "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/build_verify/_deps/can_stack-subbuild/can_stack-populate-prefix/src/can_stack-populate-stamp"
  "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/build_verify/_deps/can_stack-subbuild/can_stack-populate-prefix/src"
  "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/build_verify/_deps/can_stack-subbuild/can_stack-populate-prefix/src/can_stack-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/build_verify/_deps/can_stack-subbuild/can_stack-populate-prefix/src/can_stack-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/gr_efr/Documents/prv_sw_dev/AgIsoTaskControllerServer/build_verify/_deps/can_stack-subbuild/can_stack-populate-prefix/src/can_stack-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
