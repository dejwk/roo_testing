#include "roo_testing/transducers/ui/flex_viewport.h"

void FlexViewport::fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                            uint32_t color_argb) {
  if (bottom_to_top_) {
    std::swap(y0, y1);
    y0 = height() - y0 - 1;
    y1 = height() - y1 - 1;
  }
  if (right_to_left_) {
    std::swap(x0, x1);
    x0 = width() - x0 - 1;
    x1 = width() - x1 - 1;
  }
  if (xy_swapped_) {
    std::swap(x0, y0);
    std::swap(x1, y1);
  }
  delegate_.fillRect(x0 * magnification_, y0 * magnification_,
                     (x1 + 1) * magnification_ - 1,
                     (y1 + 1) * magnification_ - 1, color_argb);
}

FlexViewport::FlexViewport(Viewport &delegate, int magnification,
                           Rotation rotation)
    : delegate_(delegate),
      magnification_(magnification),
      xy_swapped_(rotation == kRotationRight || rotation == kRotationLeft),
      bottom_to_top_(rotation == kRotationUpsideDown ||
                     rotation == kRotationRight),
      right_to_left_(rotation == kRotationLeft ||
                     rotation == kRotationUpsideDown) {}

bool FlexViewport::isMouseClicked(int16_t *x, int16_t *y) {
  int16_t dx, dy;
  bool clicked = delegate_.isMouseClicked(&dx, &dy);
  if (!clicked) return false;
  if (xy_swapped_) {
    std::swap(dx, dy);
  }
  *x = dx * magnification_;
  *y = dy * magnification_;
  return true;
}
