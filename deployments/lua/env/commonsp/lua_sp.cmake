#-------------------------------------------------------------------------------
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Includes components needed for deploying the LUA service provider
# within a secure partition.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Common components for LUA sp deployments
#
#-------------------------------------------------------------------------------
add_components(TARGET "lua"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/fdt"
		"components/common/trace"
		"components/common/utils"
		"components/config/ramstore"
		"components/config/loader/sp"
		"components/messaging/ffa/libsp"
		"components/rpc/common/interface"
		"components/rpc/common/endpoint"
		"components/rpc/ts_rpc/common"
		"components/rpc/ts_rpc/endpoint/sp"
		"components/rpc/ts_rpc/caller/sp"
		"components/service/log/factory"
		"components/service/log/client"
)

set(CMAKE_SYSTEM_PROCESSOR arm64)

# Link SP with Lua static library
include(${TS_ROOT}/external/lua/lua.cmake)
target_link_libraries(lua PRIVATE lua_static)

target_sources(lua PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/lua_sp.c
)
