/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PACKEDC_LUA_PROVIDER_SERIALIZER_H
#define PACKEDC_LUA_PROVIDER_SERIALIZER_H

#include <service/lua/provider/serializer/lua_serializer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Singleton method to provide access to the packed-c serializer
 * for the Lua service provider.
 */
const struct lua_serializer *packedc_lua_serializer_instance(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PACKEDC_LUA_PROVIDER_SERIALIZER_H */
