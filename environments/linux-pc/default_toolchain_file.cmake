#-------------------------------------------------------------------------------
# Copyright (c) 2020-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# Since we append to default compilation flags stop multiple inclusion to avoid
# flags being added multiple times.
include_guard(GLOBAL)

set(TS_DEBUG_INFO_FLAGS "-fdiagnostics-show-option -gdwarf-2" CACHE STRING "Compiler flags to add debug information.")
set(TS_MANDATORY_AARCH_FLAGS "" CACHE STRING "Compiler flags configuring architecture specific ")
set(TS_WARNING_FLAGS "-Wall" CACHE STRING "Compiler flags affecting generating warning messages.")
set(TS_MANDATORY_LINKER_FLAGS "" CACHE STRING "Linker flags needed for correct builds.")

# Allow defining the "warning as error behavior" using an environment variable. But prioritize command line
# definition if present.
# If a cache variable is not present
if(NOT DEFINED CACHE{CMAKE_COMPILE_WARNING_AS_ERROR})
	# And an environment variable is, copy its value to the cache
	if (DEFINED ENV{CMAKE_COMPILE_WARNING_AS_ERROR})
		set(CMAKE_COMPILE_WARNING_AS_ERROR $ENV{CMAKE_COMPILE_WARNING_AS_ERROR} CACHE Bool "If compilation warnings should be treated as errors.")
	endif()
endif()

# By default warnings should be treated as errors.
set(CMAKE_COMPILE_WARNING_AS_ERROR On CACHE BOOL "If compilation warnings should be treated as errors.")

# Cmake v3.24 + shall set the warning flag automatically, but does not when processing our deployments. As a workaround
# set -Werror manually always as setting it twice shall have no ill effect.
if (CMAKE_COMPILE_WARNING_AS_ERROR)
	string(APPEND TS_WARNING_FLAGS " -Werror")
endif()

# Set flags affecting all build types
string(APPEND CMAKE_C_FLAGS_INIT " ${TS_MANDATORY_AARCH_FLAGS}")
string(APPEND CMAKE_CXX_FLAGS_INIT " ${TS_MANDATORY_AARCH_FLAGS}")
string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT " ${TS_MANDATORY_LINKER_FLAGS}")
if(DEFINED TS_ROOT)
	# Flags not to be used with external components.
	string(APPEND CMAKE_C_FLAGS_INIT " ${TS_WARNING_FLAGS}")
	string(APPEND CMAKE_CXX_FLAGS_INIT " ${TS_WARNING_FLAGS}")
endif()

# Set flags affecting all build types supporting debugging.
foreach(_b_type IN ITEMS DEBUG RELWITHDEBINFO MINSIZWITHDEBINFO DEBUGCOVERAGE)
	string(APPEND CMAKE_C_FLAGS_${_b_type}_INIT " ${TS_DEBUG_INFO_FLAGS}")
	string(APPEND CMAKE_CXX_FLAGS_${_b_type}_INIT " ${TS_DEBUG_INFO_FLAGS}")
endforeach()

# Build type specific flags
string(APPEND CMAKE_C_FLAGS_DEBUG_INIT " -O0")
string(APPEND CMAKE_C_FLAGS_MINSIZEREL_INIT  " -Os")
string(APPEND CMAKE_C_FLAGS_MINSIZWITHDEBINFO_INIT " -Os")
string(APPEND CMAKE_C_FLAGS_RELEASE_INIT " -O2")
string(APPEND CMAKE_C_FLAGS_RELWITHDEBINFO_INIT " -O2")
string(APPEND CMAKE_C_FLAGS_DEBUGCOVERAGE_INIT " -O0 -coverage")
string(APPEND CMAKE_CXX_FLAGS_DEBUG_INIT " -O0")
string(APPEND CMAKE_CXX_FLAGS_MINSIZEREL_INIT  " -Os")
string(APPEND CMAKE_CXX_FLAGS_MINSIZWITHDEBINFO_INIT " -Os")
string(APPEND CMAKE_CXX_FLAGS_RELEASE_INIT " -O2")
string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT " -O2")
string(APPEND CMAKE_CXX_FLAGS_DEBUGCOVERAGE_INIT " -O0 -coverage")

include($ENV{TS_ROOT}/tools/cmake/compiler/GCC.cmake REQUIRED)
include($ENV{TS_ROOT}/tools/cmake/compiler/config_iface.cmake REQUIRED)
