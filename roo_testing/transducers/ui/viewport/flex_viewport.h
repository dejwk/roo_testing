#pragma once

#include <algorithm>

#include "roo_testing/transducers/ui/viewport/viewport.h"

namespace roo_testing_transducers {

class FlexViewport : public Viewport {
 public:
  enum Rotation {
    kRotationNone,
    kRotationRight,
    kRotationUpsideDown,
    kRotationLeft
  };

  FlexViewport(Viewport& delegate, int magnification,
               Rotation rotation = kRotationNone);

  virtual ~FlexViewport() {}

  void init(int16_t width, int16_t height) override {
    Viewport::init(width, height);
    if (xy_swapped_) {
      std::swap(width, height);
    }
    delegate_.init(width * magnification(), height * magnification());
  }

  void fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                uint32_t color_argb) override;

  void drawRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                const uint32_t* color_argb) override;

  void flush() override { delegate_.flush(); }

  bool isMouseClicked(int16_t* x, int16_t* y) override;

  int magnification() const { return magnification_; }

 private:
  Viewport& delegate_;
  int magnification_;
  bool xy_swapped_;
  bool bottom_to_top_;
  bool right_to_left_;
};

}  // namespace roo_testing_transducers
