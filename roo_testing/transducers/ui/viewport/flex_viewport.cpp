#include "roo_testing/transducers/ui/viewport/flex_viewport.h"

namespace roo_testing_transducers {

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

void FlexViewport::drawRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                            const uint32_t* color_argb) {
  if (!bottom_to_top_ && !right_to_left_ && !xy_swapped_ &&
      magnification_ == 1) {
    delegate_.drawRect(x0, y0, x1, y1, color_argb);
    return;
  }
  // Compute transformed destination rectangle using the same logic as fillRect.
  int16_t dx0 = x0, dy0 = y0, dx1 = x1, dy1 = y1;
  if (bottom_to_top_) {
    std::swap(dy0, dy1);
    dy0 = height() - dy0 - 1;
    dy1 = height() - dy1 - 1;
  }
  if (right_to_left_) {
    std::swap(dx0, dx1);
    dx0 = width() - dx0 - 1;
    dx1 = width() - dx1 - 1;
  }
  if (xy_swapped_) {
    std::swap(dx0, dy0);
    std::swap(dx1, dy1);
  }
  int16_t w = x1 - x0 + 1;
  int16_t h = y1 - y0 + 1;
  int dw = (dx1 - dx0 + 1) * magnification_;
  int dh = (dy1 - dy0 + 1) * magnification_;
  uint32_t* dest = new uint32_t[dw * dh];
  for (int16_t sy = 0; sy < h; ++sy) {
    for (int16_t sx = 0; sx < w; ++sx) {
      uint32_t color = color_argb[sy * w + sx];
      int16_t tx = x0 + sx;
      int16_t ty = y0 + sy;
      if (bottom_to_top_) ty = height() - 1 - ty;
      if (right_to_left_) tx = width() - 1 - tx;
      if (xy_swapped_) std::swap(tx, ty);
      int dest_sx = tx - dx0;
      int dest_sy = ty - dy0;
      for (int my = 0; my < magnification_; ++my) {
        for (int mx = 0; mx < magnification_; ++mx) {
          dest[(dest_sy * magnification_ + my) * dw +
               dest_sx * magnification_ + mx] = color;
        }
      }
    }
  }
  delegate_.drawRect(dx0 * magnification_, dy0 * magnification_,
                     dx0 * magnification_ + dw - 1,
                     dy0 * magnification_ + dh - 1, dest);
}

FlexViewport::FlexViewport(Viewport& delegate, int magnification,
                           Rotation rotation)
    : delegate_(delegate),
      magnification_(magnification),
      xy_swapped_(rotation == kRotationRight || rotation == kRotationLeft),
      bottom_to_top_(rotation == kRotationUpsideDown ||
                     rotation == kRotationRight),
      right_to_left_(rotation == kRotationLeft ||
                     rotation == kRotationUpsideDown) {}

bool FlexViewport::isMouseClicked(int16_t* x, int16_t* y) {
  int16_t dx, dy;
  bool clicked = delegate_.isMouseClicked(&dx, &dy);
  if (!clicked) return false;
  dx /= magnification_;
  dy /= magnification_;
  if (xy_swapped_) {
    std::swap(dx, dy);
  }
  if (bottom_to_top_) {
    dy = height() - dy - 1;
  }
  if (right_to_left_) {
    dx = width() - dx - 1;
  }
  *x = dx;
  *y = dy;
  return true;
}

}  // namespace roo_testing_transducers
