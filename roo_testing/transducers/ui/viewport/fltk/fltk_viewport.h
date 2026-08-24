#pragma once

#include <cstdint>

#include "roo_testing/transducers/ui/viewport/viewport.h"

#ifndef FLTK_DEVICE_NOISE_BITS
#define FLTK_DEVICE_NOISE_BITS 0
#endif

#ifndef FLTK_MAX_PIXELS_PER_MS
#define FLTK_MAX_PIXELS_PER_MS 0
#endif

namespace roo_testing_transducers {

class EventQueue;

struct FltkViewportOptions {
  uint8_t noise_bits = FLTK_DEVICE_NOISE_BITS;
  int max_pixels_per_ms = FLTK_MAX_PIXELS_PER_MS;
};

class FltkViewport : public Viewport {
 public:
  explicit FltkViewport(FltkViewportOptions options = {});

  ~FltkViewport();

  void init(int16_t width, int16_t height) override;

  void flush() override;

  bool isMouseClicked(int16_t* x, int16_t* y) override;

  void fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                uint32_t color_argb) override;

  void drawRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                const uint32_t* color_argb) override;

 private:
  EventQueue* queue_;
  const FltkViewportOptions options_;
};

}  // namespace roo_testing_transducers
