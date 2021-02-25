// TFT emulator for the esp-display library.
// It uses the FLTK library to render data on the screen.

// /* Standard headers */
// #include <pthread.h>
// #include <stdlib.h>

// #include <iostream>
// #include <mutex>
// #include <queue>

// #include <Arduino.h>

// /* Fltk headers */
// #include <FL/Fl.H>
// #include <FL/Fl_Box.H>
// #include <FL/Fl_Double_Window.H>
// #include <FL/fl_draw.H>
// #include <FL/x.H>

#pragma once

#include <memory>

#include "device.h"
// #include <esp_display.h>
// #include <offscreen.h>

using namespace espdisplay;

class EventQueue;

class EmulatorDevice : public DisplayDevice {
 public:
  EmulatorDevice(int w, int h, int magnification);
  ~EmulatorDevice();

  void init() override;
  void begin() override;
  void end() override;

  void setAddress(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) override {
    addr_x0_ = x0;
    addr_y0_ = y0;
    addr_x1_ = x1;
    addr_y1_ = y1;
    cursor_x_ = x0;
    cursor_y_ = y0;
  }

  void writeColors(espdisplay::Color* colors, uint32_t pixel_count) override;

  void drawPixels(int16_t* xs, int16_t* ys, espdisplay::Color* colors,
                  uint16_t pixel_count) override;
  void paintPixels(int16_t* xs, int16_t* ys, espdisplay::Color* colors,
                   uint16_t pixel_count) override;

  void setBackground(espdisplay::Color bgcolor) {
    bgcolor_ = bgcolor;
  }

  espdisplay::Color readColor() override { return bgcolor_; }

  void fillColor(espdisplay::Color color, uint32_t pixel_count) override;

  int16_t width() const override;
  int16_t height() const override;

  int magnification() const;

 private:
  void advance();

  EventQueue* queue_;
  espdisplay::Color bgcolor_;
  int16_t width_, height_;
  int16_t addr_x0_, addr_y0_, addr_x1_, addr_y1_;
  int16_t cursor_x_, cursor_y_;
};
