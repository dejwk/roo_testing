#include "roo_testing/devices/roo_display/fltk_device.h"

#include "roo_display/core/color.h"

using namespace roo_display;

void EmulatorDevice::fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                              Color color) {
  if (orientation().isXYswapped()) {
    std::swap(x0, y0);
    std::swap(x1, y1);
  }
  if (orientation().isBottomToTop()) {
    std::swap(y0, y1);
    y0 = raw_height() - y0;
    y1 = raw_height() - y1;
  }
  if (orientation().isRightToLeft()) {
    std::swap(x0, x1);
    x0 = raw_width() - x0;
    x1 = raw_width() - x1;
  }
  framebuffer_.fillRect(x0, y0, x1, y1, color.asArgb() >> 8);
}

EmulatorDevice::EmulatorDevice(int w, int h, int magnification)
    : DisplayDevice(w, h),
      framebuffer_(w, h, magnification),
      bgcolor_(0xFF7F7F7F) {}

void EmulatorDevice::begin() {}
void EmulatorDevice::end() { framebuffer_.flush(); }

Color EmulatorDevice::effective_color(roo_display::PaintMode mode,
                                      Color color) {
  switch (mode) {
    case PAINT_MODE_REPLACE: {
      return color;
      break;
    }
    case PAINT_MODE_BLEND: {
      return alphaBlend(bgcolor_, color);
    }
    default: {
      return color;
    }
  }
}

void EmulatorDevice::write(roo_display::Color *colors, uint32_t pixel_count) {
  for (uint32_t i = 0; i < pixel_count; ++i) {
    fillRect(cursor_x_, cursor_y_, cursor_x_, cursor_y_,
             effective_color(paint_mode_, colors[i]));
    advance();
  }
}

// void EmulatorDevice::fill(PaintMode mode, roo_display::Color color,
//                           uint32_t pixel_count) {
//   for (uint32_t i = 0; i < pixel_count; ++i) {
//     rectFill(cursor_x_, cursor_y_, 1, 1, effective_color(mode, color));
//     advance();
//   }
// }

void EmulatorDevice::writeRects(PaintMode mode, Color *color, int16_t *x0,
                                int16_t *y0, int16_t *x1, int16_t *y1,
                                uint16_t count) {
  while (count-- > 0) {
    fillRect(*x0++, *y0++, *x1++, *y1++, effective_color(mode, *color++));
  }
}

void EmulatorDevice::fillRects(PaintMode mode, Color color, int16_t *x0,
                               int16_t *y0, int16_t *x1, int16_t *y1,
                               uint16_t count) {
  while (count-- > 0) {
    fillRect(*x0++, *y0++, *x1++, *y1++, effective_color(mode, color));
  }
}

void EmulatorDevice::writePixels(PaintMode mode, roo_display::Color *colors,
                                 int16_t *xs, int16_t *ys,
                                 uint16_t pixel_count) {
  for (int i = 0; i < pixel_count; ++i) {
    fillRect(xs[i], ys[i], xs[i], ys[i], effective_color(mode, colors[i]));
  }
}

void EmulatorDevice::fillPixels(PaintMode mode, roo_display::Color color,
                                int16_t *xs, int16_t *ys,
                                uint16_t pixel_count) {
  for (int i = 0; i < pixel_count; ++i) {
    fillRect(xs[i], ys[i], xs[i], ys[i], effective_color(mode, color));
  }
}

void EmulatorDevice::advance() {
  ++cursor_x_;
  if (cursor_x_ > addr_x1_) {
    cursor_x_ = addr_x0_;
    ++cursor_y_;
  }
}

bool EmulatorDevice::getTouch(int16_t *x, int16_t *y, int16_t *z) {
  int16_t x_display, y_display;
  bool result = framebuffer_.isMouseClicked(&x_display, &y_display);
  if (result) {
    *x = (int32_t)4096 * x_display / effective_width();
    *y = (int32_t)4096 * y_display / effective_height();
    *z = 100;
  }
  return result;
}

void EmulatorDevice::init() { framebuffer_.init(); }
