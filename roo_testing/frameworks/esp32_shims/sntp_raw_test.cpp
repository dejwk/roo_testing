#include <gtest/gtest.h>
extern "C" int SntpRawProbe(void);
// Verifies raw C lwIP symbols link and share the firmware client's state.
TEST(SntpRaw, HeaderAndLinkCompatibility) { EXPECT_EQ(1, SntpRawProbe()); }
