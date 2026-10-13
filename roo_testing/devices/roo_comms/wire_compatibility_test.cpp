#include <string>

#include "gtest/gtest.h"
#include "roo_testing/devices/roo_comms/roo_comms.pb.h"

// Verifies that isolating the fake's C++ package preserves its discovery wire
// format for the real roo_comms receiver.
TEST(FakeCommsProtoTest, DiscoveryRequestWireFormat) {
  roo::testing::comms::ControlMessage message;
  message.mutable_hub_discovery_request()
      ->mutable_device_descriptor()
      ->set_realm_id(roo::testing::comms::kHomeAutomation);

  std::string encoded;
  ASSERT_TRUE(message.SerializeToString(&encoded));
  EXPECT_EQ(encoded, std::string("\x0a\x04\x0a\x02\x08\x01", 6));
}
