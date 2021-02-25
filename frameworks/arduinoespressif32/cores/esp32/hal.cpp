/*
 Arduino.h - Main include file for the Arduino SDK
 Copyright (c) 2005-2013 Arduino Team.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "hal.h"
#include <thread>

#include "testing/transducers/time/clock.h"

using testing_transducers::getDefaultSystemClock;

#ifdef __cplusplus
extern "C" {
#endif

uint64_t get_timestamp() {
  return getDefaultSystemClock()->getTimeMicros();
}

static uint64_t start_time() {
  static uint64_t time = get_timestamp();
  return time;
}

int64_t esp_timer_get_time() {
  return micros();
}

unsigned long micros() { return get_timestamp() - start_time(); }

unsigned long millis() { return (get_timestamp() - start_time()) / 1000; }

void delay(uint32_t delay) {
  getDefaultSystemClock()->delayMicros(((uint64_t)delay) * 1000);
}

void delayMicroseconds(uint32_t us) {
  getDefaultSystemClock()->delayMicros(us);
}

#ifdef __cplusplus
}
#endif
