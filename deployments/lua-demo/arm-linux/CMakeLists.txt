#-------------------------------------------------------------------------------
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
include(../../deployment.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  The CMakeLists.txt for building the lua-demo deployment for arm-linux
#
#-------------------------------------------------------------------------------
include(${TS_ROOT}/environments/arm-linux/env.cmake)
project(trusted-services LANGUAGES CXX C)
add_executable(lua-demo)
target_include_directories(lua-demo PRIVATE "${TOP_LEVEL_INCLUDE_DIRS}")

#-------------------------------------------------------------------------------
#  Extend with components that are common across all deployments of
#  lua-demo
#
#-------------------------------------------------------------------------------
include(../lua-demo.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  Define library options and dependencies.
#
#-------------------------------------------------------------------------------
target_link_libraries(lua-demo PRIVATE stdc++ gcc m)
