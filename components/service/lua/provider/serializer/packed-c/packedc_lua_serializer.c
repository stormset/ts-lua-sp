/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <string.h>
#include "common/tlv/tlv.h"
#include "protocols/rpc/common/packed-c/status.h"
#include "protocols/service/lua/packed-c/lua_proto.h"
#include "packedc_lua_serializer.h"


/* Operation: env_create */
rpc_status_t serialize_env_create_resp(struct rpc_buffer *resp_buf, int32_t env_index)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct ts_lua_env_create_out resp_msg;
	size_t fixed_len = sizeof(struct ts_lua_env_create_out);

	resp_msg.env_index = env_index;

	if (fixed_len <= resp_buf->size) {
		memcpy(resp_buf->data, &resp_msg, fixed_len);
		resp_buf->data_length = fixed_len;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: env_append */
rpc_status_t deserialize_env_append_req(const struct rpc_buffer *req_buf,
	int32_t *env_index,
	const uint8_t **script,
	size_t *script_len)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	struct ts_lua_env_append_in recv_msg;
	size_t expected_fixed_len = sizeof(struct ts_lua_env_append_in);

	if (expected_fixed_len <= req_buf->data_length) {
		memcpy(&recv_msg, req_buf->data, expected_fixed_len);

		*env_index = recv_msg.env_index;

		*script = (const uint8_t *)req_buf->data + expected_fixed_len;
		*script_len = req_buf->data_length - expected_fixed_len;

		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: env_execute */
rpc_status_t deserialize_env_execute_req(const struct rpc_buffer *req_buf, int32_t *env_index)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	struct ts_lua_env_execute_in recv_msg;
	size_t expected_fixed_len = sizeof(struct ts_lua_env_execute_in);

	if (expected_fixed_len <= req_buf->data_length) {
		memcpy(&recv_msg, req_buf->data, expected_fixed_len);
		*env_index = recv_msg.env_index;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

rpc_status_t serialize_env_execute_resp(struct rpc_buffer *resp_buf,
	const uint8_t *err_msg,
	size_t err_len)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;

	if (err_len == 0) {
		resp_buf->data_length = 0;
		rpc_status = RPC_SUCCESS;
	} else if (err_len <= resp_buf->size) {
		memcpy(resp_buf->data, err_msg, err_len);
		resp_buf->data_length = err_len;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: env_delete */
rpc_status_t deserialize_env_delete_req(const struct rpc_buffer *req_buf,
	int32_t *env_index)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	struct ts_lua_env_delete_in recv_msg;
	size_t expected_fixed_len = sizeof(struct ts_lua_env_delete_in);

	if (expected_fixed_len <= req_buf->data_length) {
		memcpy(&recv_msg, req_buf->data, expected_fixed_len);
		*env_index = recv_msg.env_index;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Singleton method to provide access to the serializer instance */
const struct lua_serializer *packedc_lua_serializer_instance(void)
{
	static const struct lua_serializer instance =
	{
		serialize_env_create_resp,
		deserialize_env_append_req,
		deserialize_env_execute_req,
		serialize_env_execute_resp,
		deserialize_env_delete_req
	};

	return &instance;
}
