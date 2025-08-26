#-------------------------------------------------------------------------------
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# LUA interpreter
#-------------------------------------------------------------------------------

set(LUA_URL "https://github.com/walterschell/Lua.git" CACHE STRING "LUA repository URL")
set(LUA_REFSPEC "v5.4.5" CACHE STRING "Lua git refspec")
set(LUA_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/lua-src" CACHE PATH "Lua source directory")
set(LUA_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/lua_install" CACHE PATH "Lua install directory")
set(LUA_BUILD_TYPE "Release" CACHE STRING "Lua build type")

include(${TS_ROOT}/external/openlibm/openlibm.cmake)

set(GIT_OPTIONS
	GIT_REPOSITORY ${LUA_URL}
	GIT_TAG ${LUA_REFSPEC}
	GIT_SHALLOW FALSE
	PATCH_COMMAND
		git stash
		COMMAND git branch -f bf-am
		COMMAND git am ${CMAKE_CURRENT_LIST_DIR}/0001-Make-LUA_LIB_SRCS-a-CACHE-STRING-so-it-can-be-overri.patch
		COMMAND git am ${CMAKE_CURRENT_LIST_DIR}/0002-Adopt-string-copy-functions-to-libc.patch
		COMMAND git am ${CMAKE_CURRENT_LIST_DIR}/0003-Remove-functions-requiring-file-system-support.patch
		COMMAND git am ${CMAKE_CURRENT_LIST_DIR}/0004-Add-definitions-to-luaconf.h-to-adapt-Lua-to-deploym.patch
		COMMAND git reset bf-am
)

# Only pass libc settings to lua if needed. For environments where the standard
# library is not overridden, this is not needed.
if(TARGET stdlib::c)
	include(${TS_ROOT}/tools/cmake/common/PropertyCopy.cmake)

	# Save libc settings
	save_interface_target_properties(TGT stdlib::c PREFIX LIBC)
	# Translate libc settings to cmake code fragment. Will be inserted into
	# lua-init-cache.cmake.in when LazyFetch configures the file.
	translate_interface_target_properties(PREFIX LIBC RES _libc_fragment)
	unset_saved_properties(LIBC)
endif()

# If openlibm is a defined target capture its interface properties and pass to Lua.
if(TARGET openlibm)
	include(${TS_ROOT}/tools/cmake/common/PropertyCopy.cmake)

	save_interface_target_properties(TGT openlibm PREFIX OPENLIBM)
	translate_interface_target_properties(PREFIX OPENLIBM RES _openlibm_fragment)
	unset_saved_properties(OPENLIBM)
endif()

include(${TS_ROOT}/tools/cmake/common/LazyFetch.cmake REQUIRED)
LazyFetch_MakeAvailable(DEP_NAME lua
	FETCH_OPTIONS "${GIT_OPTIONS}"
	INSTALL_DIR "${LUA_INSTALL_DIR}"
	CACHE_FILE "${CMAKE_CURRENT_LIST_DIR}/lua-init-cache.cmake.in"
	SOURCE_DIR "${LUA_SOURCE_DIR}"
)
unset(_libc_fragment)
unset(_openlibm_fragment)


# Create an imported target to abstract the build system.
add_library(lua_static STATIC IMPORTED)
set_property(TARGET lua_static PROPERTY IMPORTED_LOCATION "${LUA_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}lua_static${CMAKE_STATIC_LIBRARY_SUFFIX}")
set_property(TARGET lua_static PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${LUA_INSTALL_DIR}/include")
if(TARGET stdlib::c)
	target_link_libraries(lua_static INTERFACE stdlib::c)
endif()
if(TARGET openlibm)
    target_link_libraries(lua_static INTERFACE openlibm)
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${LUA_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}lua_static${CMAKE_STATIC_LIBRARY_SUFFIX}")
