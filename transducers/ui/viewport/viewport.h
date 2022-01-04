#pragma once

#include <inttypes.h>

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

  virtual void flush() {}

  virtual bool isMouseClicked(int16_t* x, int16_t* y) { return false; }

  int16_t width() const { return width_; }
  int16_t height() const { return height_; }

 private:
  int16_t width_;
  int16_t height_;
};
