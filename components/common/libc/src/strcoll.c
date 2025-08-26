/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

/* 
 * This implementation of strcoll simply calls strcmp.
 * In environments where locale is not available or needed, strcmp can be used
 * as a substitute for collation, which performs a simple byte-wise comparison.
 */
int strcoll(const char *s1, const char *s2) {
    return strcmp(s1, s2);
}
