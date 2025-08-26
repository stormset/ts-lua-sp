/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 */

#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "components/rpc/common/endpoint/rpc_service_interface.h"
#include "protocols/rpc/common/packed-c/status.h"
#include "protocols/service/lua/status.h"
#include "protocols/service/lua/packed-c/lua_proto.h"
#include "lua_provider.h"
#include "lua_uuid.h"
#include "trace.h"
#include "util.h"

/* Service request handlers */
static rpc_status_t env_create_handler(void *context, struct rpc_request *req);
static rpc_status_t env_append_handler(void *context, struct rpc_request *req);
static rpc_status_t env_execute_handler(void *context, struct rpc_request *req);
static rpc_status_t env_delete_handler(void *context, struct rpc_request *req);

/* Handler mapping table for service */
static const struct service_handler handler_table[] = {
	{TS_LUA_OPCODE_ENV_CREATE,  env_create_handler},
	{TS_LUA_OPCODE_ENV_APPEND,  env_append_handler},
	{TS_LUA_OPCODE_ENV_EXECUTE, env_execute_handler},
	{TS_LUA_OPCODE_ENV_DELETE,  env_delete_handler}
};

/* C-binding for Lua's print function following Lua's luaB_print implementation */
static int lua_log(lua_State *L) {
	/* Get number of arguments */
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++) {
        size_t len;
        const char *s = luaL_tolstring(L, i, &len);

        if (i > 1)
            ts_trace_printf("", 0, TRACE_LEVEL_NONE, "%s", "\t");
        ts_trace_printf("", 0, TRACE_LEVEL_NONE, "%s", s);
		
        lua_pop(L, 1);
    }

	return 0;
}

struct rpc_service_interface *lua_provider_init(struct lua_provider *context)
{
	const struct rpc_uuid service_uuid = { .uuid = TS_LUA_SERVICE_UUID };

	if (context == NULL)
		return NULL;

	if (context->lua_state != NULL)
		lua_close(context->lua_state);

	/* Initialize Lua */
	context->lua_state = luaL_newstate();
	if (context->lua_state == NULL)
		return NULL;

	/* Load common libraries into global environment */
    luaL_requiref(context->lua_state, "_G", luaopen_base, 1);
    lua_pop(context->lua_state, 1);
    luaL_requiref(context->lua_state, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(context->lua_state, 1);
    luaL_requiref(context->lua_state, LUA_STRLIBNAME, luaopen_string, 1);
    lua_pop(context->lua_state, 1);
    luaL_requiref(context->lua_state, LUA_TABLIBNAME, luaopen_table, 1);
    lua_pop(context->lua_state, 1);
    luaL_requiref(context->lua_state, LUA_COLIBNAME, luaopen_coroutine, 1);
    lua_pop(context->lua_state, 1);
    luaL_requiref(context->lua_state, LUA_UTF8LIBNAME, luaopen_utf8, 1);
    lua_pop(context->lua_state, 1);

	/* Setup global C bindings */
	/* Make print point to lua_log that uses ts_trace_printf for logging */
	lua_pushcfunction(context->lua_state, lua_log);
	lua_setglobal(context->lua_state, "print");

	/* TODO: Add binding for LIBSP & PSA API */

    /* Create a metatable in registry. This is where environments will look up a key
     * in case they don't find a it in their own table.
	 */
    if (luaL_newmetatable(context->lua_state, "global_env_meta")) {
        /* Set new metatable (global_env_meta) __index field to point to global table */
        lua_pushglobaltable(context->lua_state);
        lua_setfield(context->lua_state, -2, "__index");
    }
    lua_pop(context->lua_state, 1);

	/* Initialize environment pool */
	struct env_entry *entry = context->env_entries;
	for (int32_t i = 0; i < MAX_ENV_COUNT - 1; ++i, ++entry) {
		entry->next = i + 1;
		entry->lua_env_ref = LUA_REFNIL;
		entry->script_buf = NULL;
		entry->script_len = 0;
	}

	entry->next = NIL;
	entry->lua_env_ref = LUA_REFNIL;
	entry->script_buf = NULL;
	entry->script_len = 0;

	context->free_env_entry_ind = 0;
	
	service_provider_init(&context->base_provider, context, &service_uuid, handler_table,
				  ARRAY_SIZE(handler_table));

	return service_provider_get_rpc_interface(&context->base_provider);
}

void lua_provider_register_serializer(
	struct lua_provider *context,
	const struct lua_serializer *serializer)
{
	context->serializer = serializer;
}

static const struct lua_serializer* get_lua_serializer(
	struct lua_provider *context,
	const struct rpc_request *req)
{
	(void) req;

	return context->serializer;
}

static rpc_status_t env_create_handler(void *context, struct rpc_request *req)
{
	struct lua_provider *this_instance = (struct lua_provider *)context;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;

	/* Get next free slot in environment buffer */
	int32_t free_idx = this_instance->free_env_entry_ind;
	if (free_idx == NIL) {
		DMSG("Limit of %" PRIu8 " environments reached.", MAX_ENV_COUNT);
		req->service_status = LUA_ERROR_OUT_OF_FREE_ENVIRONMENTS;

		return RPC_SUCCESS;
	}
	struct env_entry *entry = &this_instance->env_entries[free_idx];

	/* Update next free index */
	this_instance->free_env_entry_ind = entry->next;

	/* Create new table for environment */
	lua_newtable(this_instance->lua_state);

	/* Set new table's metatable to global_env_meta that points to the global environment */
	luaL_setmetatable(this_instance->lua_state, "global_env_meta");

	/* Create and return reference to newly created table */
	int env_ref = luaL_ref(this_instance->lua_state, LUA_REGISTRYINDEX);

	/* Initialize environment entry */
	entry->next = NIL;
	entry->lua_env_ref = env_ref;
	entry->script_buf = NULL;
	entry->script_len = 0;

	req->service_status = LUA_SUCCESS;

	const struct lua_serializer *serializer = get_lua_serializer(this_instance, req);
	if (serializer)
		rpc_status = serializer->serialize_env_create_resp(&req->response, free_idx);

	return rpc_status;
}

static rpc_status_t env_append_handler(void *context, struct rpc_request *req)
{
	struct lua_provider *this_instance = (struct lua_provider *)context;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;

	const struct lua_serializer *serializer = get_lua_serializer(this_instance, req);

	int32_t env_index = -1;
	const uint8_t *script = NULL;
	size_t script_len = 0;

	if (serializer)
		rpc_status = serializer->deserialize_env_append_req(&req->request, &env_index,
			&script, &script_len);

	if (rpc_status != RPC_SUCCESS) {
		req->service_status = LUA_ERROR_GENERIC_ERROR;

		return RPC_SUCCESS;
	}

	/* Validate environment index and check if it is initialized */
	if (env_index < 0 || env_index >= MAX_ENV_COUNT ||
		this_instance->env_entries[env_index].lua_env_ref == LUA_REFNIL) {

		req->service_status = LUA_ERROR_ENVIRONMENT_DOES_NOT_EXIST;

		return RPC_SUCCESS;
	}
	struct env_entry *entry = &this_instance->env_entries[env_index];

	/* Reallocate script buffer so new chunk fits (maybe use different growth strategy?) */
	size_t new_len = entry->script_len + script_len;
	char *tmp = (char *)realloc(entry->script_buf, new_len + 1);
	if (!tmp) {
		req->service_status = LUA_ERROR_OUT_OF_MEMORY;

		return RPC_SUCCESS;
	}

	/* Copy new chunk */
	memcpy(tmp + entry->script_len, script, script_len);

	entry->script_buf = tmp;
	entry->script_len = new_len;

	req->service_status = LUA_SUCCESS;

	return RPC_SUCCESS;
}

static rpc_status_t env_execute_handler(void *context, struct rpc_request *req)
{
	struct lua_provider *this_instance = (struct lua_provider *)context;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *resp_buf = &req->response;

	const struct lua_serializer *serializer = get_lua_serializer(this_instance, req);

	int32_t env_index = -1;

	if (serializer)
		rpc_status = serializer->deserialize_env_execute_req(&req->request, &env_index);

	if (rpc_status != RPC_SUCCESS) {
		req->service_status = LUA_ERROR_GENERIC_ERROR;

		return RPC_SUCCESS;
	}

	/* Validate environment index and check if it is initialized */
	if (env_index < 0 || env_index >= MAX_ENV_COUNT ||
		this_instance->env_entries[env_index].lua_env_ref == LUA_REFNIL) {

		req->service_status = LUA_ERROR_ENVIRONMENT_DOES_NOT_EXIST;

		return RPC_SUCCESS;
	}
	struct env_entry *entry = &this_instance->env_entries[env_index];

	/* Empty script buffer: success */
	if (!entry->script_buf || entry->script_len == 0) {
		req->service_status = LUA_SUCCESS;
		serializer->serialize_env_execute_resp(resp_buf, NULL, 0);

		return RPC_SUCCESS;
	}

	/* Pass script buffer to Lua for parsing and loading into VM stack */
	char script_name[32] = { 0 };
	snprintf(script_name, sizeof(script_name), "environment_%d", env_index);

	if (luaL_loadbuffer(this_instance->lua_state, entry->script_buf, entry->script_len, script_name) != LUA_OK) {
		/* Recover error message from stack */
		const char *msg = lua_tostring(this_instance->lua_state, -1);
		size_t msg_len = msg ? strlen(msg) : 0;

		req->service_status = LUA_ERROR_PARSER_ERROR;
		serializer->serialize_env_execute_resp(resp_buf, (const uint8_t *)msg, msg_len);

		lua_pop(this_instance->lua_state, 1);
		
		return RPC_SUCCESS;
	}

	/* Free script buffer as Lua already successfully parsed it and loaded onto stack */
	free(entry->script_buf);
	entry->script_buf = NULL;
	entry->script_len = 0;

	/* Setup script's environment */
	lua_rawgeti(this_instance->lua_state, LUA_REGISTRYINDEX, entry->lua_env_ref);
	lua_setupvalue(this_instance->lua_state, -2, 1);

	/* Execute */
	if (lua_pcall(this_instance->lua_state, 0, 0, 0) != LUA_OK) {
		const char *msg = lua_tostring(this_instance->lua_state, -1);
		size_t msg_len = msg ? strlen(msg) : 0;

		req->service_status = LUA_ERROR_INTERPRETER_ERROR;
		serializer->serialize_env_execute_resp(resp_buf, (const uint8_t *)msg, msg_len);

		lua_pop(this_instance->lua_state, 1);

		return RPC_SUCCESS;
	}

	/* Every step succeeded */
	req->service_status = LUA_SUCCESS;
	serializer->serialize_env_execute_resp(resp_buf, NULL, 0);

	return RPC_SUCCESS;
}

static rpc_status_t env_delete_handler(void *context, struct rpc_request *req)
{
	struct lua_provider *this_instance = (struct lua_provider *)context;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;

	const struct lua_serializer *serializer = get_lua_serializer(this_instance, req);

	int32_t env_index = -1;

	if (serializer)
		rpc_status = serializer->deserialize_env_delete_req(&req->request, &env_index);

	if (rpc_status != RPC_SUCCESS) {
		req->service_status = LUA_ERROR_GENERIC_ERROR;

		return RPC_SUCCESS;
	}

	/* Validate environment index and check if it is initialized */
	if (env_index < 0 || env_index >= MAX_ENV_COUNT ||
		this_instance->env_entries[env_index].lua_env_ref == LUA_REFNIL) {

		req->service_status = LUA_ERROR_ENVIRONMENT_DOES_NOT_EXIST;

		return RPC_SUCCESS;
	}
	struct env_entry *entry = &this_instance->env_entries[env_index];

	/* Unref environment so GC can cleen it up */
	luaL_unref(this_instance->lua_state, LUA_REGISTRYINDEX, entry->lua_env_ref);
	entry->lua_env_ref = LUA_REFNIL;

	/* Free script buffer */
	free(entry->script_buf);
	entry->script_buf = NULL;
	entry->script_len = 0;

	/* Update free indices */
	entry->next = this_instance->free_env_entry_ind;
	this_instance->free_env_entry_ind = env_index;

	req->service_status = LUA_SUCCESS;

	return RPC_SUCCESS;
}
