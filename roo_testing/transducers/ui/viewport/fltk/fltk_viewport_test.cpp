#include "roo_testing/transducers/ui/viewport/fltk/fltk_viewport.h"

#include "gtest/gtest.h"

namespace roo_testing_transducers {
namespace {

// Verifies every edge and inverted rectangle fails before reaching the GUI.
// This includes the empty clip and out-of-bounds pixel from the scrolling
// crash.
TEST(FltkViewportDeathTest, RejectsInvalidRectanglesOnCallerThread) {
  FltkViewport viewport;
  // Set only the base geometry: these tests never register a GUI device or
  // start its consumer thread, so failures must come from the drawing call.
  viewport.Viewport::init(320, 240);
  const int16_t rectangles[][4] = {
      {-1, 0, 0, 0}, {0, -1, 0, 0},     {319, 0, 320, 0},    {0, 239, 0, 240},
      {1, 0, 0, 0},  {12, 64, 307, 62}, {101, 277, 101, 277}};
  for (const auto &rect : rectangles) {
    EXPECT_DEATH(viewport.fillRect(rect[0], rect[1], rect[2], rect[3], 0),
                 "Invalid viewport rectangle.*320x240");
    EXPECT_DEATH(viewport.drawRect(rect[0], rect[1], rect[2], rect[3], nullptr),
                 "Invalid viewport rectangle.*320x240");
  }
}

// Verifies missing initialization and invalid dimensions fail without opening
// a window or attempting a framebuffer allocation.
TEST(FltkViewportDeathTest, RejectsUninitializedDrawingAndInvalidSizes) {
  FltkViewport viewport;
  EXPECT_DEATH(viewport.fillRect(0, 0, 0, 0, 0), "Invalid viewport rectangle");
  EXPECT_DEATH(viewport.drawRect(0, 0, 0, 0, nullptr),
               "Invalid viewport rectangle");
  EXPECT_DEATH(viewport.init(0, 240), "Invalid viewport dimensions 0x240");
  EXPECT_DEATH(viewport.init(320, 0), "Invalid viewport dimensions 320x0");
  EXPECT_DEATH(viewport.init(-1, 240), "Invalid viewport dimensions -1x240");
  EXPECT_DEATH(viewport.init(320, -1), "Invalid viewport dimensions 320x-1");
}

// Verifies inclusive boundary coordinates and one-pixel displays remain valid.
TEST(FltkViewportTest, AcceptsFullDisplayAndBoundaryPixels) {
  // No consumer is registered; the local pixel stays alive until the queued
  // messages are destroyed with the viewport.
  const uint32_t pixel = 0xFFFFFFFF;
  FltkViewport viewport;
  viewport.Viewport::init(320, 240);
  viewport.fillRect(0, 0, 319, 239, pixel);
  for (int16_t x : {0, 319}) {
    for (int16_t y : {0, 239}) {
      viewport.fillRect(x, y, x, y, pixel);
      viewport.drawRect(x, y, x, y, &pixel);
    }
  }
  viewport.Viewport::init(1, 1);
  viewport.fillRect(0, 0, 0, 0, pixel);
  viewport.drawRect(0, 0, 0, 0, &pixel);
}

} // namespace
} // namespace roo_testing_transducers
