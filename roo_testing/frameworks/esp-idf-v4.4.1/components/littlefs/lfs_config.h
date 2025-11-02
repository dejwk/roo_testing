/*
 * lfs utility functions
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef LFS_CFG_H
#define LFS_CFG_H

// System includes
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "esp_log.h"


#if defined(CONFIG_LITTLEFS_MALLOC_STRATEGY_DEFAULT) || \
    defined(CONFIG_LITTLEFS_MALLOC_STRATEGY_INTERNAL) || \
    defined(CONFIG_LITTLEFS_MALLOC_STRATEGY_SPIRAM)
#include <stdlib.h>
#include "esp_heap_caps.h"
#endif

#ifdef CONFIG_LITTLEFS_ASSERTS
#include <assert.h>
#endif

#if !defined(LFS_NO_DEBUG) || \
        !defined(LFS_NO_WARN) || \
        !defined(LFS_NO_ERROR) || \
        defined(LFS_YES_TRACE)
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif


// Macros, may be replaced by system specific wrappers. Arguments to these
// macros must not have side-effects as the macros can be removed for a smaller
// code footprint
extern const char ESP_LITTLEFS_TAG[];

// Logging functions
#ifndef LFS_TRACE
#ifdef LFS_YES_TRACE
#define LFS_TRACE_(fmt, ...) \
    ESP_LOGV(ESP_LITTLEFS_TAG, "%s:%d:trace: " fmt "%s\n", __FILE__, __LINE__, __VA_ARGS__)
#define LFS_TRACE(...) LFS_TRACE_(__VA_ARGS__, "")
#else
#define LFS_TRACE(...)
#endif
#endif

#ifndef LFS_DEBUG
#ifndef LFS_NO_DEBUG
#define LFS_DEBUG_(fmt, ...) \
    ESP_LOGD(ESP_LITTLEFS_TAG, "%s:%d:debug: " fmt "%s\n", __FILE__, __LINE__, __VA_ARGS__)
#define LFS_DEBUG(...) LFS_DEBUG_(__VA_ARGS__, "")
#else
#define LFS_DEBUG(...)
#endif
#endif

#ifndef LFS_WARN
#ifndef LFS_NO_WARN
#define LFS_WARN_(fmt, ...) \
    ESP_LOGW(ESP_LITTLEFS_TAG, "%s:%d:warn: " fmt "%s\n", __FILE__, __LINE__, __VA_ARGS__)
#define LFS_WARN(...) LFS_WARN_(__VA_ARGS__, "")
#else
#define LFS_WARN(...)
#endif
#endif

#ifndef LFS_ERROR
#ifndef LFS_NO_ERROR
#define LFS_ERROR_(fmt, ...) \
    ESP_LOGE(ESP_LITTLEFS_TAG, "%s:%d:error: " fmt "%s\n", __FILE__, __LINE__, __VA_ARGS__)
#define LFS_ERROR(...) LFS_ERROR_(__VA_ARGS__, "")
#else
#define LFS_ERROR(...)
#endif
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif