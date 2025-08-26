/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TS_LUA_PACKEDC_MESSAGES_H
#define TS_LUA_PACKEDC_MESSAGES_H

#include <stdint.h>


/****************************************
 * \brief env_create operation
 *
 * Create a new Lua environment for script isolation.
 * On success env_index is an index identifying the new environment.
 * On failure env_index will be -1.
 ****************************************/

struct __attribute__((__packed__)) ts_lua_env_create_out {
    int32_t env_index;
};

/****************************************
 * \brief env_append operation
 *
 * Append script to the specified environment's script buffer.
 * This will be passed to Lua for parsing and interpretation by calling env_execute.
 ****************************************/
struct __attribute__((__packed__)) ts_lua_env_append_in {
    int32_t env_index;
};

/****************************************
 * \brief env_execute operation
 *
 * Call Lua's parser & interpreter on the environment's script buffer.
 * The return value of the parser or interpreter (if parser succeeds) will be in service_status.
 * In case of error the error message will be returned in the payload.
 ****************************************/

struct __attribute__((__packed__)) ts_lua_env_execute_in {
    int32_t env_index;
};

/****************************************
 * \brief env_delete operation
 *
 * Delete the specified Lua environment.
 ****************************************/

struct __attribute__((__packed__)) ts_lua_env_delete_in {
    int32_t env_index;
};

#define TS_LUA_OPCODE_BASE          (0x200u)

#define TS_LUA_OPCODE_ENV_CREATE    (TS_LUA_OPCODE_BASE + 0u)
#define TS_LUA_OPCODE_ENV_APPEND    (TS_LUA_OPCODE_BASE + 1u)
#define TS_LUA_OPCODE_ENV_EXECUTE   (TS_LUA_OPCODE_BASE + 2u)
#define TS_LUA_OPCODE_ENV_DELETE    (TS_LUA_OPCODE_BASE + 3u)

#endif /* TS_LUA_PACKEDC_MESSAGES_H */
