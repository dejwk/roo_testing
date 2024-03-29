#pragma once

#include <memory>

#include "roo_display/core/device.h"
#include "roo_testing/transducers/ui/viewport.h"

class ReferenceDevice : public roo_display::DisplayDevice,
                        public roo_display::TouchDevice {
 public:
  ReferenceDevice(int w, int h, Viewport& viewport);

  ~ReferenceDevice() {}

  void init() override;
  void begin() override;
  void end() override;

  void setAddress(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                  roo_display::PaintMode mode) override {
    addr_x0_ = x0;
    addr_y0_ = y0;
    addr_x1_ = x1;
    addr_y1_ = y1;
    paint_mode_ = mode;
    cursor_x_ = x0;
    cursor_y_ = y0;
  }

  void write(roo_display::Color* colors, uint32_t pixel_count) override;

  //  void fill(roo_display::PaintMode mode, roo_display::Color color, uint32_t
  //  pixel_count) override;

  void writeRects(roo_display::PaintMode mode, roo_display::Color* color,
                  int16_t* x0, int16_t* y0, int16_t* x1, int16_t* y1,
                  uint16_t count) override;

  void fillRects(roo_display::PaintMode mode, roo_display::Color color,
                 int16_t* x0, int16_t* y0, int16_t* x1, int16_t* y1,
                 uint16_t count) override;

  void writePixels(roo_display::PaintMode mode, roo_display::Color* colors,
                   int16_t* xs, int16_t* ys, uint16_t pixel_count) override;

  void fillPixels(roo_display::PaintMode mode, roo_display::Color color,
                  int16_t* xs, int16_t* ys, uint16_t pixel_count) override;

  void setBgColorHint(roo_display::Color bgcolor) override {
    bgcolor_ = bgcolor;
  }

  bool getTouch(int16_t* x, int16_t* y, int16_t* z) override;

 private:
  roo_display::Color effective_color(roo_display::PaintMode mode,
                                     roo_display::Color color);

  void fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                roo_display::Color color);

  void advance();

  Viewport& viewport_;
  roo_display::Color bgcolor_;
  int16_t addr_x0_, addr_y0_, addr_x1_, addr_y1_;
  roo_display::PaintMode paint_mode_;
  int16_t cursor_x_, cursor_y_;
};
