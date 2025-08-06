// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "multi_heap.h"
#include "esp_log.h"

// // forward declaration
// IRAM_ATTR static void *heap_caps_realloc_base( void *ptr, size_t size, uint32_t caps);

// /*
// This file, combined with a region allocator that supports multiple heaps, solves the problem that the ESP32 has RAM
// that's slightly heterogeneous. Some RAM can be byte-accessed, some allows only 32-bit accesses, some can execute memory,
// some can be remapped by the MMU to only be accessed by a certain PID etc. In order to allow the most flexible memory
// allocation possible, this code makes it possible to request memory that has certain capabilities. The code will then use
// its knowledge of how the memory is configured along with a priority scheme to allocate that memory in the most sane way
// possible. This should optimize the amount of RAM accessible to the code without hardwiring addresses.
// */

static esp_alloc_failed_hook_t alloc_failed_callback;


static void heap_caps_alloc_failed(size_t requested_size, uint32_t caps, const char *function_name)
{
    if (alloc_failed_callback) {
        alloc_failed_callback(requested_size, caps, function_name);
    }

    #ifdef CONFIG_HEAP_ABORT_WHEN_ALLOCATION_FAILS
    esp_system_abort("Memory allocation failed");
    #endif
}

esp_err_t heap_caps_register_failed_alloc_callback(esp_alloc_failed_hook_t callback)
{
    if (callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    alloc_failed_callback = callback;

    return ESP_OK;
}

/*
Routine to allocate a bit of memory with certain capabilities. caps is a bitfield of MALLOC_CAP_* bits.
*/
IRAM_ATTR void *heap_caps_malloc( size_t size, uint32_t caps){

    // void* ptr = heap_caps_malloc_base(size, caps);

    void* ptr = malloc(size);

    if (!ptr){
        heap_caps_alloc_failed(size, caps, __func__);
    }

    return ptr;
}

IRAM_ATTR void heap_caps_free( void *ptr) {
    free(ptr);
}

IRAM_ATTR void *heap_caps_realloc( void *ptr, size_t size, uint32_t caps)
{
    // ptr = heap_caps_realloc_base(ptr, size, caps);

    ptr = realloc(ptr, size);

    if (ptr == NULL && size > 0){
        heap_caps_alloc_failed(size, caps, __func__);
    }

    return ptr;
}

IRAM_ATTR void *heap_caps_calloc( size_t n, size_t size, uint32_t caps)
{
    void *result;
    size_t size_bytes;

    if (__builtin_mul_overflow(n, size, &size_bytes)) {
        return NULL;
    }

    // result = heap_caps_malloc(size_bytes, caps);

    result = malloc(size_bytes);

    if (result != NULL) {
        bzero(result, size_bytes);
    }
    return result;
}
