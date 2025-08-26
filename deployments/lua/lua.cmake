#-------------------------------------------------------------------------------
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

add_components(TARGET "lua"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		components/rpc/common/caller
		components/service/common/include
		components/service/common/client
		components/service/common/provider
		components/service/lua/provider
		components/service/lua/provider/serializer/packed-c
		protocols/rpc/common/packed-c
)

target_include_directories(lua PRIVATE
	${TS_ROOT}
	${TS_ROOT}/components
)
