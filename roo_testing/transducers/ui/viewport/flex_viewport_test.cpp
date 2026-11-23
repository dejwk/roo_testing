#include "roo_testing/transducers/ui/viewport/flex_viewport.h"

#include <array>
#include <memory>
#include <utility>

#include "gtest/gtest.h"

namespace roo_testing_transducers {
namespace {

/// Retains the owned rectangle and pixels for inspection after drawing.
class RecordingViewport : public Viewport {
 public:
  void fillRect(int16_t, int16_t, int16_t, int16_t, uint32_t) override {
    ADD_FAILURE() << "Expected an array of pixels";
  }

  /// Captures the submitted bounds and takes ownership of the pixel array.
  void drawRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                std::unique_ptr<const uint32_t[]> color_argb) override {
    bounds = {x0, y0, x1, y1};
    pixels = std::move(color_argb);
  }

  std::array<int16_t, 4> bounds{};
  std::unique_ptr<const uint32_t[]> pixels;
};

// Verifies that the identity path forwards ownership of the original array.
TEST(FlexViewportTest, ForwardsOriginalBuffer) {
  RecordingViewport delegate;
  FlexViewport viewport(delegate, 1);
  viewport.init(4, 3);
  std::unique_ptr<uint32_t[]> pixels(new uint32_t[2]{1, 2});
  const uint32_t* original = pixels.get();

  viewport.drawRect(1, 1, 2, 1, std::move(pixels));

  EXPECT_EQ(pixels, nullptr);
  EXPECT_EQ(delegate.pixels.get(), original);
  EXPECT_EQ(delegate.bounds, (std::array<int16_t, 4>{1, 1, 2, 1}));
}

// Verifies the transformed buffer survives the call and has the right pixels.
// LeakSanitizer also checks that the original array is released.
TEST(FlexViewportTest, TransfersRotatedAndScaledBuffer) {
  RecordingViewport delegate;
  FlexViewport viewport(delegate, 2, FlexViewport::kRotationRight);
  viewport.init(3, 3);
  std::unique_ptr<uint32_t[]> pixels(new uint32_t[2]{1, 2});

  viewport.drawRect(1, 0, 2, 0, std::move(pixels));

  EXPECT_EQ(pixels, nullptr);
  EXPECT_EQ(delegate.bounds, (std::array<int16_t, 4>{4, 2, 5, 5}));
  ASSERT_NE(delegate.pixels, nullptr);
  const std::array<uint32_t, 8> expected = {1, 1, 1, 1, 2, 2, 2, 2};
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(delegate.pixels[i], expected[i]);
  }
}

}  // namespace
}  // namespace roo_testing_transducers
