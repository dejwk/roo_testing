#include "roo_testing/devices/roo_display/reference_device.h"

#include "roo_display/core/color.h"

using namespace roo_display;

void ReferenceDevice::fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                               Color color) {
  if (orientation().isXYswapped()) {
    std::swap(x0, y0);
    std::swap(x1, y1);
  }
  if (orientation().isBottomToTop()) {
    std::swap(y0, y1);
    y0 = raw_height() - y0 - 1;
    y1 = raw_height() - y1 - 1;
  }
  if (orientation().isRightToLeft()) {
    std::swap(x0, x1);
    x0 = raw_width() - x0 - 1;
    x1 = raw_width() - x1 - 1;
  }
  viewport_.fillRect(x0, y0, x1, y1, color.asArgb());
}

ReferenceDevice::ReferenceDevice(int w, int h, Viewport &viewport)
    : DisplayDevice(w, h), viewport_(viewport), bgcolor_(0xFF7F7F7F) {
  // viewport_.init(w, h);
}

void ReferenceDevice::begin() {}
void ReferenceDevice::end() { viewport_.flush(); }

Color ReferenceDevice::effective_color(roo_display::PaintMode mode,
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

void ReferenceDevice::write(roo_display::Color *colors, uint32_t pixel_count) {
  for (uint32_t i = 0; i < pixel_count; ++i) {
    fillRect(cursor_x_, cursor_y_, cursor_x_, cursor_y_,
             effective_color(paint_mode_, colors[i]));
    advance();
  }
}

// void ReferenceDevice::fill(PaintMode mode, roo_display::Color color,
//                           uint32_t pixel_count) {
//   for (uint32_t i = 0; i < pixel_count; ++i) {
//     rectFill(cursor_x_, cursor_y_, 1, 1, effective_color(mode, color));
//     advance();
//   }
// }

void ReferenceDevice::writeRects(PaintMode mode, Color *color, int16_t *x0,
                                 int16_t *y0, int16_t *x1, int16_t *y1,
                                 uint16_t count) {
  while (count-- > 0) {
    fillRect(*x0++, *y0++, *x1++, *y1++, effective_color(mode, *color++));
  }
}

void ReferenceDevice::fillRects(PaintMode mode, Color color, int16_t *x0,
                                int16_t *y0, int16_t *x1, int16_t *y1,
                                uint16_t count) {
  while (count-- > 0) {
    fillRect(*x0++, *y0++, *x1++, *y1++, effective_color(mode, color));
  }
}

void ReferenceDevice::writePixels(PaintMode mode, roo_display::Color *colors,
                                  int16_t *xs, int16_t *ys,
                                  uint16_t pixel_count) {
  for (int i = 0; i < pixel_count; ++i) {
    fillRect(xs[i], ys[i], xs[i], ys[i], effective_color(mode, colors[i]));
  }
}

void ReferenceDevice::fillPixels(PaintMode mode, roo_display::Color color,
                                 int16_t *xs, int16_t *ys,
                                 uint16_t pixel_count) {
  for (int i = 0; i < pixel_count; ++i) {
    fillRect(xs[i], ys[i], xs[i], ys[i], effective_color(mode, color));
  }
}

void ReferenceDevice::advance() {
  ++cursor_x_;
  if (cursor_x_ > addr_x1_) {
    cursor_x_ = addr_x0_;
    ++cursor_y_;
  }
}

bool ReferenceDevice::getTouch(int16_t *x, int16_t *y, int16_t *z) {
  int16_t x_display, y_display;
  bool result = viewport_.isMouseClicked(&x_display, &y_display);
  if (result) {
    *x = (int32_t)4096 * x_display / effective_width();
    *y = (int32_t)4096 * y_display / effective_height();
    *z = 100;
  }
  return result;
}

void ReferenceDevice::init() { viewport_.init(raw_width(), raw_height()); }
