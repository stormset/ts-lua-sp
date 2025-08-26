/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LUA_PROVIDER_SERIALIZER_H
#define LUA_PROVIDER_SERIALIZER_H

#include <stddef.h>
#include <stdint.h>
#include "common/uuid/uuid.h"
#include "components/rpc/common/endpoint/rpc_service_interface.h"

/* Provides a common interface for parameter serialization operations
 * for the Lua service provider.  Allows alternative serialization
 * protocols to be used without hard-wiring a particular protocol
 * into the service provider code.  A concrete serializer must
 * implement this interface.
 */
struct lua_serializer {

	/* Operation: env_create */
	rpc_status_t (*serialize_env_create_resp)(struct rpc_buffer *resp_buf, int32_t env_index);

	/* Operation: env_append */
	rpc_status_t (*deserialize_env_append_req)(const struct rpc_buffer *req_buf,
		int32_t *env_index,
		const uint8_t **script,
		size_t *script_len);

	/* Operation: env_execute */
	rpc_status_t (*deserialize_env_execute_req)(const struct rpc_buffer *req_buf, int32_t *env_index);

	rpc_status_t (*serialize_env_execute_resp)(struct rpc_buffer *resp_buf,
		const uint8_t *err_msg,
		size_t err_len);

	/* Operation: env_delete */
	rpc_status_t (*deserialize_env_delete_req)(const struct rpc_buffer *req_buf, int32_t *env_index);
};

#endif /* LUA_PROVIDER_SERIALIZER_H */
