/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 */

#ifndef LUA_PROVIDER_H
#define LUA_PROVIDER_H

#include <lua.h>

#include "components/rpc/common/endpoint/rpc_service_interface.h"
#include "components/service/common/provider/service_provider.h"
#include "serializer/lua_serializer.h"
#include "protocols/rpc/common/packed-c/encoding.h"

#define MAX_ENV_COUNT ((uint8_t)(16))
#define NIL (-1)

#ifdef __cplusplus
extern "C" {
#endif

struct env_entry {
    /* reference to the Lua environment table */
    int lua_env_ref;
    /* contains concatenated script to be interpreted by Lua */
    char *script_buf;
    /* current script length in bytes */
    size_t script_len;
    /* link to next free index */
    int32_t next;
};

struct lua_provider {
	struct service_provider base_provider;
    const struct lua_serializer *serializer;
    lua_State *lua_state;
    struct env_entry env_entries[MAX_ENV_COUNT];
    int32_t free_env_entry_ind;
};

struct rpc_service_interface *lua_provider_init(struct lua_provider *context);

void lua_provider_register_serializer(
	struct lua_provider *context,
	const struct lua_serializer *serializer);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LUA_PROVIDER_H */
