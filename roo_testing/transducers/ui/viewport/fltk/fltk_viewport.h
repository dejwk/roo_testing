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

  /// Opens a display with positive @p width and @p height; rejects invalid
  /// sizes.
  void init(int16_t width, int16_t height) override;

  void flush() override;

  bool isMouseClicked(int16_t *x, int16_t *y) override;

  /// Queues a solid fill within the initialized display.
  /// Terminates with a diagnostic if the inclusive rectangle is empty or
  /// outside the display, before submitting work to the GUI thread.
  void fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                uint32_t color_argb) override;

  /// Queues pixel data for a rectangle within the initialized display.
  /// Applies the same rectangle validation as fillRect().
  void drawRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                const uint32_t *color_argb) override;

 private:
  /// Rejects invalid drawing bounds on the caller thread, before queueing.
  void validateRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1) const;

  EventQueue *queue_;
  const FltkViewportOptions options_;
};

}  // namespace roo_testing_transducers
