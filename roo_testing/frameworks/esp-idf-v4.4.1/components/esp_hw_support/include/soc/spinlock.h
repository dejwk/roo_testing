/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "hal/assert.h"
#include <assert.h>

#include <pthread.h>

#include <stdint.h>
#include <stdbool.h>

# define XTOS_SET_INTLEVEL(intlevel)		0
# define XTOS_SET_MIN_INTLEVEL(intlevel)	0
# define XTOS_RESTORE_INTLEVEL(restoreval)
# define XTOS_RESTORE_JUST_INTLEVEL(restoreval)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SPIRAM_WORKAROUND_NEED_VOLATILE_SPINLOCK
#define NEED_VOLATILE_MUX volatile
#else
#define NEED_VOLATILE_MUX
#endif

#define SPINLOCK_FREE          0xB33FFFFF
#define SPINLOCK_WAIT_FOREVER  (-1)
#define SPINLOCK_NO_WAIT        0
#define SPINLOCK_INITIALIZER   PTHREAD_MUTEX_INITIALIZER
#define CORE_ID_REGVAL_XOR_SWAP (0xCDCD ^ 0xABAB)

typedef struct {
    pthread_mutex_t mutex;
}spinlock_t;

/**
 * @brief Initialize a lock to its default state - unlocked
 * @param lock - spinlock object to initialize
 */
static inline void __attribute__((always_inline)) spinlock_initialize(spinlock_t *lock)
{
    assert(lock);
#if !CONFIG_FREERTOS_UNICORE
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&lock->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
#endif
}

/**
 * @brief Top level spinlock acquire function, spins until get the lock
 *
 * This function will:
 * - Save current interrupt state, then disable interrupts
 * - Spin until lock is acquired or until timeout occurs
 * - Restore interrupt state
 *
 * @note Spinlocks alone do no constitute true critical sections (as this
 *       function reenables interrupts once the spinlock is acquired). For critical
 *       sections, use the interface provided by the operating system.
 * @param lock - target spinlock object
 * @param timeout - cycles to wait, passing SPINLOCK_WAIT_FOREVER blocs indefinitely
 */
static inline bool __attribute__((always_inline)) spinlock_acquire(spinlock_t *lock, int32_t timeout)
{
#if !CONFIG_FREERTOS_UNICORE && !BOOTLOADER_BUILD
    uint32_t irq_status;

    assert(lock);
    irq_status = XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL);
    (void)irq_status;

    if (timeout == SPINLOCK_WAIT_FOREVER) {
        pthread_mutex_lock(&lock->mutex);
        return true;
    } else {
        // 80 MHz -> 12.5ns per cycle
        struct timespec spec = {
            .tv_nsec = 25 * timeout / 2,
        };
        bool acquired = pthread_mutex_timedlock(&lock->mutex, &spec);
        if (!acquired) {
            XTOS_RESTORE_INTLEVEL(irq_status);
            return false;
        }
    }
    XTOS_RESTORE_INTLEVEL(irq_status);
    return true;

#else  // !CONFIG_FREERTOS_UNICORE
    return true;
#endif
}

/**
 * @brief Top level spinlock unlock function, unlocks a previously locked spinlock
 *
 * This function will:
 * - Save current interrupt state, then disable interrupts
 * - Release the spinlock
 * - Restore interrupt state
 *
 * @note Spinlocks alone do no constitute true critical sections (as this
 *       function reenables interrupts once the spinlock is acquired). For critical
 *       sections, use the interface provided by the operating system.
 * @param lock - target, locked before, spinlock object
 */
static inline void __attribute__((always_inline)) spinlock_release(spinlock_t *lock)
{
#if !CONFIG_FREERTOS_UNICORE && !BOOTLOADER_BUILD
     uint32_t irq_status;
//     uint32_t core_id;

    assert(lock);
    irq_status = XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL);
    (void)irq_status;
    pthread_mutex_unlock(&lock->mutex);

    XTOS_RESTORE_INTLEVEL(irq_status);
#endif
}

#ifdef __cplusplus
}
#endif
