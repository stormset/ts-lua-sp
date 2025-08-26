/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef LUA_CLIENT_H
#define LUA_CLIENT_H

#include "service/common/client/service_client.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief lua_client structure
 *
 * A lua_client instance is used communicates with a remote lua provider.
 */
struct lua_client {
	struct service_client client;
};

/**
 * @brief Initialize a lua_client
 *
 * A lua client is a lua backend that makes RPC calls
 * to a remote lua provider.
 *
 * @param[in]  context	The subject lua_client
 * @param[in]  rpc_caller An rpc_caller for reaching the associated service provider
 *
 *
 * @return	Pointer to inialized lua backend or NULL on failure
 */
void lua_client_init(struct lua_client *lua_client, struct rpc_caller_session *session);

/**
 * \brief De-initialize a lua_client
 *
 *  Frees resource allocated during call to lua_client_init().
 *
 * \param[in]  lua_client  The subject lua_client
 */
void lua_client_deinit(struct lua_client *lua_client);

/**
 * \brief Create a new Lua environment
 *
 * Create a new Lua environment for script isolation.
 * On success env_index is an index identifying the new environment.
 * On failure env_index will be -1.
 *
 * \param[in]  context     Pointer to lua_client
 * \param[out] env_index   Index of created environment
 *
 * \return PSA_SUCCESS on success, or other PSA error codes
 */
psa_status_t env_create(void *context, int32_t *env_index);

/**
 * \brief Append script to an environment's script buffer
 *
 * Append script to the specified environment's script buffer.
 * This will be passed to Lua for parsing and interpretation by calling env_execute.
 *
 * \param[in]  context       Pointer to lua_client
 * \param[in]  env_index     Index of target environment
 * \param[in]  script_bytes  Script bytes to be appended
 * \param[in]  script_len    Length of script_bytes array
 *
 * \return corresponding PSA-API status codes
 */
psa_status_t env_append(void *context, int32_t env_index,
                        const uint8_t *script_bytes, size_t script_len);

/**
 * \brief Parse and interpret the environment's script buffer
 *
 * Call Lua's parser & interpreter on the environment's script buffer.
 * The return value of the parser or interpreter (if parser succeeds) will be in service_status.
 * In case of error the error message will be returned in the payload.
 *
 * \param[in]  context             Pointer to lua_client
 * \param[in]  env_index           Index of environment to execute
 * \param[in]  error_msg_buf_size  Size of input buffer for storing error message
 * \param[out] error_msg_buf       Buffer for error message
 * \param[out] error_msg_len       Length of the error message
 *
 * \return corresponding PSA-API status codes
 */
psa_status_t env_execute(void *context, int32_t env_index,
                         size_t error_msg_buf_size,
                         uint8_t *error_msg_buf,
                         size_t *error_msg_len);

/**
 * \brief Delete the specified Lua environment
 *
 * \param[in]  context     Pointer to lua_client
 * \param[in]  env_index   Index of environment to delete
 *
 * \return corresponding PSA-API status codes
 */
psa_status_t env_delete(void *context, int32_t env_index);

#ifdef __cplusplus
}
#endif

#endif /* LUA_CLIENT_H */
