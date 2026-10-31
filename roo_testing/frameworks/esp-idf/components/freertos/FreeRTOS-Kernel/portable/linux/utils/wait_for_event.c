/*
 * SPDX-FileCopyrightText: 2021 Amazon.com, Inc. or its affiliates
 *
 * SPDX-License-Identifier: MIT
 */
/*
 * FreeRTOS Kernel V10.4.6
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#include "wait_for_event.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

struct event {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  bool event_triggered;
};

// Native waits must remain on the host monotonic clock when device UTC steps.
static void CheckPthread(int result) {
  if (result != 0) abort();
}

static void EventUnlockAfterCancel(void* mutex) {
  CheckPthread(pthread_mutex_unlock(mutex));
}

struct event* event_create(void) {
  struct event* ev = malloc(sizeof(struct event));
  if (ev == NULL) abort();
  ev->event_triggered = false;
  CheckPthread(pthread_mutex_init(&ev->mutex, NULL));
  pthread_condattr_t attributes;
  CheckPthread(pthread_condattr_init(&attributes));
  CheckPthread(pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC));
  CheckPthread(pthread_cond_init(&ev->cond, &attributes));
  CheckPthread(pthread_condattr_destroy(&attributes));
  return ev;
}

void event_delete(struct event* ev) {
  CheckPthread(pthread_mutex_destroy(&ev->mutex));
  CheckPthread(pthread_cond_destroy(&ev->cond));
  free(ev);
}

bool event_wait(struct event* ev) {
  CheckPthread(pthread_mutex_lock(&ev->mutex));
  pthread_cleanup_push(EventUnlockAfterCancel, &ev->mutex);
  while (!ev->event_triggered)
    CheckPthread(pthread_cond_wait(&ev->cond, &ev->mutex));
  ev->event_triggered = false;
  pthread_cleanup_pop(1);
  return true;
}

bool event_wait_timed(struct event* ev, time_t ms) {
  if (ms < 0) return false;
  struct timespec deadline;
  CheckPthread(clock_gettime(CLOCK_MONOTONIC, &deadline));
  deadline.tv_sec += ms / 1000;
  deadline.tv_nsec += (ms % 1000) * 1000000;
  deadline.tv_sec += deadline.tv_nsec / 1000000000;
  deadline.tv_nsec %= 1000000000;
  int result = 0;
  bool triggered;
  CheckPthread(pthread_mutex_lock(&ev->mutex));
  pthread_cleanup_push(EventUnlockAfterCancel, &ev->mutex);
  while (!ev->event_triggered && result == 0) {
    result = pthread_cond_timedwait(&ev->cond, &ev->mutex, &deadline);
  }
  if (result != 0 && result != ETIMEDOUT) abort();
  triggered = ev->event_triggered;
  ev->event_triggered = false;
  pthread_cleanup_pop(1);
  return triggered;
}

void event_signal(struct event* ev) {
  CheckPthread(pthread_mutex_lock(&ev->mutex));
  ev->event_triggered = true;
  CheckPthread(pthread_cond_signal(&ev->cond));
  CheckPthread(pthread_mutex_unlock(&ev->mutex));
}
