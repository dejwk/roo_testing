#include "gtest/gtest.h"
#include "roo_testing_host_sockets.h"

namespace {

// This relocation is intentionally a bare libc socket reference. GNU ld's
// --wrap option rewrites it to __wrap_socket; keeping this test independent of
// lwip_socket/roo_testing_socket catches bridge archive-retention regressions.
auto *const kBareSocketReference = &::socket;

TEST(PosixLwipBareSocketLinkTest, RetainsSocketWrapperImplementation) {
  EXPECT_NE(kBareSocketReference, nullptr);
}

}  // namespace
