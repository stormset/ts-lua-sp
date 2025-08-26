/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LUA_STATUS_H
#define LUA_STATUS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t lua_status_t;

#define LUA_SUCCESS			                 ((lua_status_t) 0)
#define LUA_ERROR_GENERIC_ERROR              ((lua_status_t)-1)
#define LUA_ERROR_OUT_OF_FREE_ENVIRONMENTS   ((lua_status_t)-2)
#define LUA_ERROR_ENVIRONMENT_DOES_NOT_EXIST ((lua_status_t)-3)
#define LUA_ERROR_OUT_OF_MEMORY              ((lua_status_t)-4)
#define LUA_ERROR_BUFFER_TOO_SMALL           ((lua_status_t)-5)
#define LUA_ERROR_PARSER_ERROR               ((lua_status_t)-6)
#define LUA_ERROR_INTERPRETER_ERROR          ((lua_status_t)-7)

#ifdef __cplusplus
}
#endif

#endif /* LUA_STATUS_H */
