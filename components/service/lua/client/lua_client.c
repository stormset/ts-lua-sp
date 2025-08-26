// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 */

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include "rpc_caller_session.h"
#include "lua_client.h"
#include "protocols/rpc/common/packed-c/status.h"
#include "protocols/service/lua/status.h"
#include "protocols/service/lua/packed-c/lua_proto.h"

/*
 * Create a new Lua environment for script isolation.
 * On success env_index is an index identifying the new environment.
 * On failure env_index will be -1.
 */
lua_status_t env_create(void *context, int32_t *env_index)
{
	struct lua_client *this_context = (struct lua_client *)context;
	lua_status_t lua_status = LUA_ERROR_GENERIC_ERROR;
	size_t req_len = 0;
	uint8_t *req_buf = NULL;

	*env_index = -1;

	rpc_call_handle call_handle =
		rpc_caller_session_begin(this_context->client.session, &req_buf, req_len,
					sizeof(struct ts_lua_env_create_out));

	if (call_handle) {
		uint8_t *resp_buf = NULL;
		size_t resp_len = 0;
		service_status_t service_status = 0;

		this_context->client.rpc_status = rpc_caller_session_invoke(
			call_handle, TS_LUA_OPCODE_ENV_CREATE, &resp_buf, &resp_len,
			&service_status);

		if (this_context->client.rpc_status == RPC_SUCCESS) {
			lua_status = service_status;

			if (lua_status == LUA_SUCCESS) {
				if (resp_len == sizeof(struct ts_lua_env_create_out)) {
					struct ts_lua_env_create_out resp_msg;

					memcpy(&resp_msg, resp_buf, sizeof(resp_msg));

					*env_index = resp_msg.env_index;
				} else {
					/* Failed to decode response message */
					lua_status = LUA_ERROR_GENERIC_ERROR;
				}
			}
		}

		rpc_caller_session_end(call_handle);
	} else {
		this_context->client.rpc_status = RPC_ERROR_INTERNAL;
	}

	return lua_status;
}

/*
 * Append script bytes to the specified environment's script buffer.
 * This will be passed to Lua for parsing and interpretation by calling env_execute.
 */
lua_status_t env_append(void *context,
	int32_t env_index,
	const uint8_t *script_bytes,
	size_t script_len)
{
	struct lua_client *this_context = (struct lua_client *)context;
	lua_status_t lua_status = LUA_ERROR_GENERIC_ERROR;
	struct ts_lua_env_append_in req_msg = {0};
	size_t req_len = sizeof(req_msg) + script_len;
	uint8_t *req_buf = NULL;

	req_msg.env_index = env_index;

	rpc_call_handle call_handle =
		rpc_caller_session_begin(this_context->client.session, &req_buf, req_len, 0);

	if (call_handle) {
		uint8_t *resp_buf = NULL;
		size_t resp_len = 0;
		service_status_t service_status = 0;

		/* Copy fixed size message */
		memcpy(req_buf, &req_msg, sizeof(req_msg));

		/* Copy variable length script */
		memcpy(&req_buf[sizeof(req_msg)], script_bytes, script_len);

		this_context->client.rpc_status = rpc_caller_session_invoke(
			call_handle, TS_LUA_OPCODE_ENV_APPEND,
			&resp_buf, &resp_len, &service_status);

		if (this_context->client.rpc_status == RPC_SUCCESS) {
			lua_status = service_status;
		}

		rpc_caller_session_end(call_handle);
	} else {
		this_context->client.rpc_status = RPC_ERROR_INTERNAL;
	}

	return lua_status;
}

/*
 * Call Lua's parser & interpreter on the environment's script buffer.
 * The return value of the parser or interpreter (if parser succeeds) will be in service_status.
 * In case of error the error message will be returned in error_msg_buf.
 */
lua_status_t env_execute(void *context,
	int32_t env_index,
	size_t error_msg_buf_size,
	uint8_t *error_msg_buf,
	size_t *error_msg_len)
{
	struct lua_client *this_context = (struct lua_client *)context;
	lua_status_t lua_status = LUA_ERROR_GENERIC_ERROR;
	struct ts_lua_env_execute_in req_msg = {0};
	size_t req_len = sizeof(req_msg);
	uint8_t *req_buf = NULL;

	*error_msg_len = 0;

	req_msg.env_index = env_index;

	rpc_call_handle call_handle =
		rpc_caller_session_begin(this_context->client.session, &req_buf, req_len,
					error_msg_buf_size);

	if (call_handle) {
		uint8_t *resp_buf = NULL;
		size_t resp_len = 0;
		service_status_t service_status = 0;

		memcpy(req_buf, &req_msg, req_len);

		this_context->client.rpc_status = rpc_caller_session_invoke(
			call_handle, TS_LUA_OPCODE_ENV_EXECUTE, &resp_buf, &resp_len,
			&service_status);

		if (this_context->client.rpc_status == RPC_SUCCESS) {
			lua_status = service_status;

			/* In failure error message from Lua is returned in response buffer */
			if (lua_status != LUA_SUCCESS) {

				if (resp_len <= error_msg_buf_size) {

					memcpy(error_msg_buf, resp_buf, resp_len);
					*error_msg_len = resp_len;
				} else {

					lua_status = PSA_ERROR_BUFFER_TOO_SMALL;
				}
			}
		}

		rpc_caller_session_end(call_handle);
	} else {

		this_context->client.rpc_status = RPC_ERROR_INTERNAL;
	}

	return lua_status;
}

/*
 * Delete the specified Lua environment.
 */
lua_status_t env_delete(void *context, int32_t env_index)
{
	struct lua_client *this_context = (struct lua_client *)context;
	lua_status_t lua_status = LUA_ERROR_GENERIC_ERROR;
	struct ts_lua_env_delete_in req_msg = {0};
	size_t req_len = sizeof(req_msg);
	uint8_t *req_buf = NULL;

	req_msg.env_index = env_index;

	rpc_call_handle call_handle =
		rpc_caller_session_begin(this_context->client.session, &req_buf, req_len, 0);

	if (call_handle) {
		uint8_t *resp_buf = NULL;
		size_t resp_len = 0;
		service_status_t service_status = 0;

		memcpy(req_buf, &req_msg, req_len);

		this_context->client.rpc_status = rpc_caller_session_invoke(
			call_handle, TS_LUA_OPCODE_ENV_DELETE,
			&resp_buf, &resp_len, &service_status);

		if (this_context->client.rpc_status == RPC_SUCCESS)
			lua_status = service_status;

		rpc_caller_session_end(call_handle);
	} else {
		this_context->client.rpc_status = RPC_ERROR_INTERNAL;
	}

	return lua_status;
}

void lua_client_init(struct lua_client *lua_client,
	struct rpc_caller_session *session)
{
	service_client_init(&lua_client->client, session);
}

void lua_client_deinit(struct lua_client *lua_client)
{
	service_client_deinit(&lua_client->client);
}
