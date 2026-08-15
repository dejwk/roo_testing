#include <atomic>
#include <thread>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "roo_testing/host_event/host_event_endpoint.h"
#include "gtest/gtest.h"

namespace {

struct State {
  roo_testing::HostEventEndpoint endpoint;
  SemaphoreHandle_t delivered = nullptr;
  std::atomic<int> calls{0};
};

void OnReady(void *context) {
  auto *state = static_cast<State *>(context);
  state->calls.fetch_add(1, std::memory_order_relaxed);
  xSemaphoreGive(state->delivered);
}

TEST(HostEventGateway, DeliversNativeReadinessFromFreeRtosTask) {
  State state;
  StaticSemaphore_t delivered_storage;
  state.delivered = xSemaphoreCreateBinaryStatic(&delivered_storage);
  ASSERT_NE(nullptr, state.delivered);
  ASSERT_EQ(roo_testing::HostEventConnectResult::kConnected,
            state.endpoint.connect(OnReady, &state));

  std::thread producer([&state] { state.endpoint.notifyFromHost(); });
  producer.join();

  EXPECT_EQ(pdTRUE, xSemaphoreTake(state.delivered, pdMS_TO_TICKS(1000)));
  EXPECT_EQ(1, state.calls.load(std::memory_order_relaxed));
  state.endpoint.disconnect();
  vSemaphoreDelete(state.delivered);
}

TEST(HostEventGateway, CoalescesSeveralNativeNotifications) {
  State state;
  StaticSemaphore_t delivered_storage;
  state.delivered = xSemaphoreCreateBinaryStatic(&delivered_storage);
  ASSERT_NE(nullptr, state.delivered);
  ASSERT_EQ(roo_testing::HostEventConnectResult::kConnected,
            state.endpoint.connect(OnReady, &state));

  std::thread producer([&state] {
    for (int i = 0; i < 32; ++i)
      state.endpoint.notifyFromHost();
  });
  producer.join();

  ASSERT_EQ(pdTRUE, xSemaphoreTake(state.delivered, pdMS_TO_TICKS(1000)));
  EXPECT_EQ(1, state.calls.load(std::memory_order_relaxed));
  EXPECT_EQ(pdFALSE, xSemaphoreTake(state.delivered, pdMS_TO_TICKS(20)));
  state.endpoint.disconnect();
  vSemaphoreDelete(state.delivered);
}

TEST(HostEventGateway, ReusesDisconnectedSlots) {
  roo_testing::HostEventEndpoint endpoints[33];
  for (int i = 0; i < 32; ++i) {
    EXPECT_EQ(roo_testing::HostEventConnectResult::kConnected,
              endpoints[i].connect(OnReady, nullptr));
  }
  EXPECT_EQ(roo_testing::HostEventConnectResult::kNoCapacity,
            endpoints[32].connect(OnReady, nullptr));

  endpoints[0].disconnect();
  EXPECT_EQ(roo_testing::HostEventConnectResult::kConnected,
            endpoints[32].connect(OnReady, nullptr));
  for (roo_testing::HostEventEndpoint &endpoint : endpoints) {
    endpoint.disconnect();
  }
}

TEST(HostEventGateway, RejectsRegistrationFromNativeThread) {
  roo_testing::HostEventEndpoint endpoint;
  std::atomic<roo_testing::HostEventConnectResult> result{
      roo_testing::HostEventConnectResult::kConnected};
  std::thread native_thread(
      [&] { result.store(endpoint.connect(OnReady, nullptr)); });
  native_thread.join();

  EXPECT_EQ(roo_testing::HostEventConnectResult::kWrongContext, result.load());
}

} // namespace
