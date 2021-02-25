/*
 HardwareSerial.h - Hardware serial library for Wiring
 Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

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

 Modified 28 September 2010 by Mark Sproul
 Modified 14 August 2012 by Alarus
 Modified 3 December 2013 by Matthijs Kooijman
 Modified 18 December 2014 by Ivan Grokhotkov (esp8266 platform support)
 Modified 31 March 2015 by Markus Sattler (rewrite the code for UART0 + UART1
 support in ESP8266) Modified 25 April 2015 by Thomas Flayols (add configuration
 different from 8N1 in ESP8266)
 */

#ifndef Serial_h
#define Serial_h

#include <inttypes.h>
#include <iostream>

#include "Stream.h"

class HardwareSerial : public Stream {
 public:
  HardwareSerial() {}

  void begin(unsigned long baud) { baud_rate_ = baud; }
  void end() {}
  int available(void) { return std::cin.peek() != EOF; }
  int peek(void) { return std::cin.peek(); }
  int read(void) { return std::cin.get(); }
  void flush(void) { std::cout << std::flush; }
  size_t write(uint8_t c) {
    std::cout.put(c);
    return 1;
  }
  size_t write(const uint8_t *buffer, size_t size) {
    std::cout.write((const char*)buffer, size);
    return size;
  }

  inline size_t write(const char *s) { return write((uint8_t *)s, strlen(s)); }
  inline size_t write(unsigned long n) { return write((uint8_t)n); }
  inline size_t write(long n) { return write((uint8_t)n); }
  inline size_t write(unsigned int n) { return write((uint8_t)n); }
  inline size_t write(int n) { return write((uint8_t)n); }
  uint32_t baudRate() { return baud_rate_; }
  operator bool() const { return true; }

  void setDebugOutput(bool) {}

 private:
  unsigned long baud_rate_;
};

extern HardwareSerial Serial;

#endif
