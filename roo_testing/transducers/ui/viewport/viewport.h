#pragma once

#include <inttypes.h>

#include <memory>

namespace roo_testing_transducers {

class Viewport {
 public:
  Viewport() : width_(-1), height_(-1) {}

  virtual ~Viewport() {}

  virtual void init(int16_t width, int16_t height) {
    width_ = width;
    height_ = height;
  }

  virtual void fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                        uint32_t color_argb) = 0;

  /// Draws the inclusive rectangle from a row-major ARGB pixel array.
  /// Takes ownership of @p color_argb, which contains one pixel per
  /// coordinate. Implementations may retain it for deferred drawing.
  virtual void drawRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                        std::unique_ptr<const uint32_t[]> color_argb) = 0;

  virtual void flush() {}

  virtual bool isMouseClicked(int16_t* x, int16_t* y) { return false; }

  int16_t width() const { return width_; }
  int16_t height() const { return height_; }

 private:
  int16_t width_;
  int16_t height_;
};

}  // namespace roo_testing_transducers
