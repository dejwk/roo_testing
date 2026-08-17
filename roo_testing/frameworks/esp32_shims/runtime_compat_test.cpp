#include <cstring>

#include "esp_err.h"
#include "esp_netif.h"
#include "freertos/ringbuf.h"
#include "gtest/gtest.h"

namespace {

TEST(RuntimeCompatTest, UsesImportedIdfErrorNames) {
  EXPECT_STREQ(esp_err_to_name(ESP_OK), "ESP_OK");
  EXPECT_STREQ(esp_err_to_name(ESP_ERR_INVALID_ARG), "ESP_ERR_INVALID_ARG");

  char name[32] = {};
  EXPECT_EQ(esp_err_to_name_r(ESP_ERR_TIMEOUT, name, sizeof(name)), name);
  EXPECT_STREQ(name, "ESP_ERR_TIMEOUT");
}

TEST(RuntimeCompatTest, ImportedByteRingBufferPreservesDataAndAccounting) {
  RingbufHandle_t ring = xRingbufferCreate(16, RINGBUF_TYPE_BYTEBUF);
  ASSERT_NE(ring, nullptr);
  EXPECT_EQ(xRingbufferGetMaxItemSize(ring), 16);
  EXPECT_EQ(xRingbufferGetCurFreeSize(ring), 16);

  constexpr char data[] = "abcdef";
  ASSERT_EQ(xRingbufferSend(ring, data, sizeof(data) - 1, 0), pdTRUE);
  EXPECT_EQ(xRingbufferGetCurFreeSize(ring), 10);

  UBaseType_t waiting = 0;
  vRingbufferGetInfo(ring, nullptr, nullptr, nullptr, nullptr, &waiting);
  EXPECT_EQ(waiting, 6);

  size_t received_size = 0;
  void* received = xRingbufferReceiveUpTo(ring, &received_size, 0, 4);
  ASSERT_NE(received, nullptr);
  ASSERT_EQ(received_size, 4);
  EXPECT_EQ(std::memcmp(received, "abcd", received_size), 0);
  vRingbufferReturnItem(ring, received);

  received = xRingbufferReceiveUpTo(ring, &received_size, 0, 16);
  ASSERT_NE(received, nullptr);
  ASSERT_EQ(received_size, 2);
  EXPECT_EQ(std::memcmp(received, "ef", received_size), 0);
  vRingbufferReturnItem(ring, received);
  EXPECT_EQ(xRingbufferGetCurFreeSize(ring), 16);

  vRingbufferDelete(ring);
}

TEST(RuntimeCompatTest, HostNetifRetainsConfiguredIpEventIds) {
  esp_netif_inherent_config_t inherent = {};
  inherent.flags = ESP_NETIF_FLAG_AUTOUP;
  inherent.get_ip_event = 1234;
  inherent.lost_ip_event = 5678;
  inherent.if_key = "TEST_NETIF";
  inherent.if_desc = "test network interface";

  esp_netif_config_t config = {};
  config.base = &inherent;
  esp_netif_t* netif = esp_netif_new(&config);
  ASSERT_NE(netif, nullptr);
  EXPECT_EQ(esp_netif_get_event_id(netif, ESP_NETIF_IP_EVENT_GOT_IP), 1234);
  EXPECT_EQ(esp_netif_get_event_id(netif, ESP_NETIF_IP_EVENT_LOST_IP), 5678);
  EXPECT_EQ(esp_netif_get_event_id(
                netif, static_cast<esp_netif_ip_event_type_t>(-1)),
            -1);
  esp_netif_destroy(netif);
}

TEST(RuntimeCompatTest, ClassifiesIdfIpv6AddressKinds) {
  esp_ip6_addr_t address = {};

  address.addr[0] = esp_netif_htonl(0x20010db8U);
  EXPECT_EQ(esp_netif_ip6_get_addr_type(&address), ESP_IP6_ADDR_IS_GLOBAL);

  address = {};
  address.addr[0] = esp_netif_htonl(0xfe800000U);
  EXPECT_EQ(esp_netif_ip6_get_addr_type(&address),
            ESP_IP6_ADDR_IS_LINK_LOCAL);

  address = {};
  address.addr[0] = esp_netif_htonl(0xfec00000U);
  EXPECT_EQ(esp_netif_ip6_get_addr_type(&address),
            ESP_IP6_ADDR_IS_SITE_LOCAL);

  address = {};
  address.addr[0] = esp_netif_htonl(0xfd000000U);
  EXPECT_EQ(esp_netif_ip6_get_addr_type(&address),
            ESP_IP6_ADDR_IS_UNIQUE_LOCAL);

  address = {};
  address.addr[2] = esp_netif_htonl(0x0000ffffU);
  EXPECT_EQ(esp_netif_ip6_get_addr_type(&address),
            ESP_IP6_ADDR_IS_IPV4_MAPPED_IPV6);

  address = {};
  address.addr[3] = esp_netif_htonl(1);
  EXPECT_EQ(esp_netif_ip6_get_addr_type(&address), ESP_IP6_ADDR_IS_UNKNOWN);
  EXPECT_EQ(esp_netif_ip6_get_addr_type(nullptr), ESP_IP6_ADDR_IS_UNKNOWN);
}

}  // namespace
