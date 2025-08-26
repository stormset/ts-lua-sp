#-------------------------------------------------------------------------------
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# OpenLibm math library
#-------------------------------------------------------------------------------

set(OPENLIBM_URL "https://github.com/JuliaMath/openlibm.git" CACHE STRING "OpenLibm repository URL")
set(OPENLIBM_REFSPEC "v0.8.7" CACHE STRING "OpenLibm git refspec")
set(OPENLIBM_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/openlibm-src" CACHE PATH "OpenLibm source directory")
set(OPENLIBM_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/openlibm_install" CACHE PATH "OpenLibm install directory")
set(OPENLIBM_BUILD_TYPE "Release" CACHE STRING "OpenLibm build type")

set(GIT_OPTIONS
	GIT_REPOSITORY ${OPENLIBM_URL}
	GIT_TAG ${OPENLIBM_REFSPEC}
	GIT_SHALLOW FALSE
	PATCH_COMMAND
		git stash
		COMMAND git branch -f bf-am
		COMMAND git am ${CMAKE_CURRENT_LIST_DIR}/0001-Use-get_filename_component-instead-of-cmake_path-for.patch
		COMMAND git am ${CMAKE_CURRENT_LIST_DIR}/0002-Make-OPENLIBM_ARCH_FOLDER-CACHE-STRING-so-target-arc.patch
		COMMAND git reset bf-am
)

# Only pass libc settings to openlibm if needed. For environments where the standard
# library is not overridden, this is not needed.
if(TARGET stdlib::c)
	include(${TS_ROOT}/tools/cmake/common/PropertyCopy.cmake)

	# Save libc settings
	save_interface_target_properties(TGT stdlib::c PREFIX LIBC)
	# Translate libc settings to cmake code fragment. Will be inserted into
	# openlibm-init-cache.cmake.in when LazyFetch configures the file.
	translate_interface_target_properties(PREFIX LIBC RES _cmake_fragment)
	unset_saved_properties(LIBC)
endif()

include(${TS_ROOT}/tools/cmake/common/LazyFetch.cmake REQUIRED)
LazyFetch_MakeAvailable(DEP_NAME openlibm
	FETCH_OPTIONS "${GIT_OPTIONS}"
	INSTALL_DIR "${OPENLIBM_INSTALL_DIR}"
	CACHE_FILE "${CMAKE_CURRENT_LIST_DIR}/openlibm-init-cache.cmake.in"
	SOURCE_DIR "${OPENLIBM_SOURCE_DIR}"
)
unset(_cmake_fragment)

# Create wrapper (math.h) around openlibm's header (openlibm_math.h)
set(OPENLIBM_MATH_HEADER_PATH "${OPENLIBM_INSTALL_DIR}/include/math.h")
set(OPENLIBM_MATH_HEADER_CONTENT [=[
#ifndef MATH_H
#define MATH_H

#include "openlibm_math.h"

#endif /* MATH_H */
]=])

if(NOT EXISTS "${OPENLIBM_MATH_HEADER_PATH}")
	file(WRITE "${OPENLIBM_MATH_HEADER_PATH}" "${OPENLIBM_MATH_HEADER_CONTENT}\n")
endif()

# Create an imported target to have clean abstraction in the build-system.
add_library(openlibm STATIC IMPORTED)
set_property(TARGET openlibm PROPERTY IMPORTED_LOCATION "${OPENLIBM_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}openlibm${CMAKE_STATIC_LIBRARY_SUFFIX}")
set_property(TARGET openlibm PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${OPENLIBM_INSTALL_DIR}/include")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${OPENLIBM_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}openlibm${CMAKE_STATIC_LIBRARY_SUFFIX}")
