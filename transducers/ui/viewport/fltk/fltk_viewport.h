#pragma once

#include <memory>
#include "roo_testing/transducers/ui/viewport/viewport.h"

namespace roo_testing_transducers {

class EventQueue;

class FltkViewport : public Viewport {
 public:
  FltkViewport();

  ~FltkViewport();

  void init(int16_t width, int16_t height) override;

  void flush() override;

  bool isMouseClicked(int16_t* x, int16_t* y) override;

  void fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                uint32_t color_argb) override;

 private:
  EventQueue* queue_;
};

}  // namespace roo_testing_transducers
