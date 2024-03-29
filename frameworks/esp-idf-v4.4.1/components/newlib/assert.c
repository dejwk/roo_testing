// Copyright 2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string.h>
#include <sys/param.h>
// #include "esp_system.h"
// #include "esp_spi_flash.h"
// #include "soc/soc_memory_layout.h"

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

char* itoa(int num, char* str, int base);

#define ASSERT_STR              "assert failed: "
#define CACHE_DISABLED_STR      "<cached disabled>"

// #if __XTENSA__
// #define INST_LEN         3
// #elif __riscv
// #define INST_LEN         4
// #endif
#define INST_LEN 4

static inline void ra_to_str(char *addr)
{
    addr[0] = '0';
    addr[1] = 'x';
    itoa((uint64_t)(__builtin_return_address(0) - INST_LEN), addr + 2, 16);
}

static void __attribute__((noreturn)) esp_system_abort(const char* buf) {
  printf(buf);
  exit(1);
}

/* Overriding assert function so that whenever assert is called from critical section,
 * it does not lead to a crash of its own.
 */
void __attribute__((noreturn)) __assert_func(const char *file, int line, const char *func, const char *expr)
{
#if CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_SILENT
    char buff[sizeof(ASSERT_STR) + 11 + 1] = ASSERT_STR;

    ra_to_str(&buff[sizeof(ASSERT_STR) - 1]);

    esp_system_abort(buff);
#else
    char addr[11] = { 0 };
    char buff[200];
    char lbuf[5];
    uint32_t rem_len = sizeof(buff) - 1;
    uint32_t off = 0;

    itoa(line, lbuf, 10);

    // if (!spi_flash_cache_enabled()) {
    //    if (esp_ptr_in_drom(file)) {
    //        file = CACHE_DISABLED_STR;
    //    }

    //    if (esp_ptr_in_drom(func)) {
    //        ra_to_str(addr);
    //        func = addr;
    //    }

    //    if (esp_ptr_in_drom(expr)) {
    //        expr = CACHE_DISABLED_STR;
    //    }
    // }

    const char *str[] = {ASSERT_STR, func ? func : "\b", " ", file, ":", lbuf, " (", expr, ")"};

    for (int i = 0; i < sizeof(str) / sizeof(str[0]); i++) {
        uint32_t len = strlen(str[i]);
        uint32_t cpy_len = MIN(len, rem_len);
        memcpy(buff + off, str[i], cpy_len);
        rem_len -= cpy_len;
        off += cpy_len;
        if (rem_len == 0) {
            break;
        }
    }
    buff[off] = '\0';
    esp_system_abort(buff);
#endif  /* CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_SILENT */
}

// void __attribute__((noreturn)) __assert(const char *file, int line, const char *failedexpr)
// {
//     __assert_func(file, line, NULL, failedexpr);
// }

/* No-op function, used to force linker to include these changes */
void newlib_include_assert_impl(void)
{
}
