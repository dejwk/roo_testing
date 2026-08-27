#include "esp_intr_alloc.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <thread>

#include "freertos/FreeRTOS.h"
#include "gtest/gtest.h"
#include "roo_testing/frameworks/esp_idf_support/esp_interrupts.h"

namespace {

constexpr int kTestSource = 12345;
constexpr int kOtherSource = -12345;
constexpr int kSharedSource = 12346;
constexpr int kCompatibilitySource = 12347;
constexpr int kRaceSource = 12348;
constexpr uint32_t kStatusAddressA = 0x60000000;
constexpr uint32_t kStatusAddressB = 0x60000004;

struct HandlerState {
  volatile sig_atomic_t count = 0;
  volatile sig_atomic_t observed_isr = 0;
};

void InterruptHandler(void* argument) {
  HandlerState* state = static_cast<HandlerState*>(argument);
  state->count = state->count + 1;
  state->observed_isr = xPortInIsrContext() == pdTRUE ? 1 : -1;
}

bool SpinUntil(volatile sig_atomic_t* value, sig_atomic_t expected) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (*value != expected && std::chrono::steady_clock::now() < deadline) {
  }
  return *value == expected;
}

void DelayedSourceRaise() {
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  roo_testing::esp_idf::raiseInterruptSource(kTestSource);
}

std::thread StartDelayedSourceRaise() {
  // Native helper threads inherit a blocked signal mask so only the selected
  // FreeRTOS pthread can receive scheduler and peripheral signals.
  portENTER_CRITICAL(nullptr);
  std::thread raiser(DelayedSourceRaise);
  portEXIT_CRITICAL(nullptr);
  return raiser;
}

}  // namespace

// Verifies source routing, ISR context, disable/enable latching, and free.
TEST(EspInterruptAllocator, RoutesSourceLifecycle) {
  HandlerState state;
  intr_handle_t handle = nullptr;
  ASSERT_EQ(esp_intr_alloc(kTestSource, 0, InterruptHandler, &state, &handle),
            ESP_OK);
  ASSERT_NE(handle, nullptr);

  roo_testing::esp_idf::raiseInterruptSource(kOtherSource);
  EXPECT_EQ(state.count, 0);
  std::thread raiser = StartDelayedSourceRaise();
  const bool handled = SpinUntil(&state.count, 1);
  raiser.join();
  EXPECT_TRUE(handled);
  EXPECT_EQ(state.observed_isr, 1);

  EXPECT_EQ(esp_intr_disable(handle), ESP_OK);
  roo_testing::esp_idf::raiseInterruptSource(kTestSource);
  EXPECT_EQ(state.count, 1);
  EXPECT_EQ(esp_intr_enable(handle), ESP_OK);
  EXPECT_TRUE(SpinUntil(&state.count, 2));

  EXPECT_EQ(esp_intr_free(handle), ESP_OK);
  roo_testing::esp_idf::raiseInterruptSource(kTestSource);
  EXPECT_EQ(state.count, 2);
  EXPECT_EQ(esp_intr_enable(handle), ESP_ERR_INVALID_ARG);
}

// Verifies initially-disabled allocation preserves an asserted source.
TEST(EspInterruptAllocator, HonorsInitiallyDisabledFlag) {
  HandlerState state;
  intr_handle_t handle = nullptr;
  ASSERT_EQ(esp_intr_alloc(kTestSource, ESP_INTR_FLAG_INTRDISABLED,
                           InterruptHandler, &state, &handle),
            ESP_OK);

  roo_testing::esp_idf::raiseInterruptSource(kTestSource);
  EXPECT_EQ(state.count, 0);
  EXPECT_EQ(esp_intr_enable(handle), ESP_OK);
  EXPECT_TRUE(SpinUntil(&state.count, 1));
  EXPECT_EQ(esp_intr_free(handle), ESP_OK);
}

// Verifies one source assertion filters and dispatches a compatible shared
// handler chain without dereferencing target MMIO addresses on the host.
TEST(EspInterruptAllocator, FiltersSharedHandlersFromStatusSnapshots) {
  constexpr int kFlags =
      ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL2;
  HandlerState first;
  HandlerState second;
  HandlerState unconditional;
  intr_handle_t first_handle = nullptr;
  intr_handle_t second_handle = nullptr;
  intr_handle_t unconditional_handle = nullptr;
  ASSERT_EQ(
      esp_intr_alloc_intrstatus(kSharedSource, kFlags, kStatusAddressA, 0x01,
                                InterruptHandler, &first, &first_handle),
      ESP_OK);
  ASSERT_EQ(
      esp_intr_alloc_intrstatus(kSharedSource, kFlags, kStatusAddressB, 0x02,
                                InterruptHandler, &second, &second_handle),
      ESP_OK);
  ASSERT_EQ(esp_intr_alloc(kSharedSource, kFlags, InterruptHandler,
                           &unconditional, &unconditional_handle),
            ESP_OK);

  roo_testing::esp_idf::raiseInterruptSource(kSharedSource);
  EXPECT_TRUE(SpinUntil(&unconditional.count, 1));
  EXPECT_EQ(first.count, 0);
  EXPECT_EQ(second.count, 0);

  const roo_testing::esp_idf::InterruptStatusSnapshot first_status[] = {
      {kStatusAddressA, 0x01}, {kStatusAddressB, 0x00}};
  roo_testing::esp_idf::raiseInterruptSource(kSharedSource, first_status,
                                             std::size(first_status));
  EXPECT_TRUE(SpinUntil(&first.count, 1));
  EXPECT_TRUE(SpinUntil(&unconditional.count, 2));
  EXPECT_EQ(second.count, 0);

  const roo_testing::esp_idf::InterruptStatusSnapshot second_status[] = {
      {kStatusAddressB, 0x02}};
  roo_testing::esp_idf::raiseInterruptSource(kSharedSource, second_status,
                                             std::size(second_status));
  EXPECT_TRUE(SpinUntil(&second.count, 1));
  EXPECT_TRUE(SpinUntil(&unconditional.count, 3));
  EXPECT_EQ(first.count, 1);
  EXPECT_EQ(first.observed_isr, 1);
  EXPECT_EQ(second.observed_isr, 1);
  EXPECT_EQ(unconditional.observed_isr, 1);

  EXPECT_EQ(esp_intr_free(first_handle), ESP_OK);
  EXPECT_EQ(esp_intr_free(second_handle), ESP_OK);
  EXPECT_EQ(esp_intr_free(unconditional_handle), ESP_OK);
}

// Verifies intrstatus metadata has no filtering effect on non-shared vectors,
// matching ESP-IDF's direct non-shared dispatch path.
TEST(EspInterruptAllocator, IgnoresIntrstatusForNonSharedHandler) {
  HandlerState state;
  intr_handle_t handle = nullptr;
  ASSERT_EQ(esp_intr_alloc_intrstatus(kTestSource, 0, kStatusAddressA, 0x01,
                                      InterruptHandler, &state, &handle),
            ESP_OK);

  roo_testing::esp_idf::raiseInterruptSource(kTestSource);
  EXPECT_TRUE(SpinUntil(&state.count, 1));
  EXPECT_EQ(esp_intr_free(handle), ESP_OK);
}

// Verifies disabling one shared handler does not disable its compatible peers
// or retain a private callback after the shared-vector pass.
TEST(EspInterruptAllocator, DisablesIndividualSharedHandler) {
  constexpr int kFlags = ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_LEVEL1;
  HandlerState first;
  HandlerState second;
  intr_handle_t first_handle = nullptr;
  intr_handle_t second_handle = nullptr;
  ASSERT_EQ(esp_intr_alloc(kSharedSource, kFlags, InterruptHandler, &first,
                           &first_handle),
            ESP_OK);
  ASSERT_EQ(esp_intr_alloc(kSharedSource, kFlags, InterruptHandler, &second,
                           &second_handle),
            ESP_OK);

  EXPECT_EQ(esp_intr_disable(first_handle), ESP_OK);
  roo_testing::esp_idf::raiseInterruptSource(kSharedSource);
  EXPECT_TRUE(SpinUntil(&second.count, 1));
  EXPECT_EQ(first.count, 0);
  EXPECT_EQ(esp_intr_enable(first_handle), ESP_OK);
  EXPECT_EQ(first.count, 0);

  roo_testing::esp_idf::raiseInterruptSource(kSharedSource);
  EXPECT_TRUE(SpinUntil(&first.count, 1));
  EXPECT_TRUE(SpinUntil(&second.count, 2));

  EXPECT_EQ(esp_intr_free(first_handle), ESP_OK);
  EXPECT_EQ(esp_intr_free(second_handle), ESP_OK);
}

// Verifies an existing source vector cannot be duplicated or mixed between
// shared and non-shared allocation modes.
TEST(EspInterruptAllocator, RejectsDuplicateOrMixedSourceVector) {
  HandlerState first;
  HandlerState second;
  intr_handle_t first_handle = nullptr;
  ASSERT_EQ(esp_intr_alloc(kCompatibilitySource, 0, InterruptHandler, &first,
                           &first_handle),
            ESP_OK);

  intr_handle_t sentinel = reinterpret_cast<intr_handle_t>(uintptr_t{1});
  intr_handle_t output = sentinel;
  EXPECT_EQ(esp_intr_alloc(kCompatibilitySource, 0, InterruptHandler, &second,
                           &output),
            ESP_ERR_NOT_FOUND);
  EXPECT_EQ(output, sentinel);
  EXPECT_EQ(esp_intr_alloc(kCompatibilitySource, ESP_INTR_FLAG_SHARED,
                           InterruptHandler, &second, &output),
            ESP_ERR_NOT_FOUND);
  EXPECT_EQ(output, sentinel);
  EXPECT_EQ(esp_intr_free(first_handle), ESP_OK);

  intr_handle_t shared_handle = nullptr;
  ASSERT_EQ(esp_intr_alloc(kCompatibilitySource, ESP_INTR_FLAG_SHARED,
                           InterruptHandler, &first, &shared_handle),
            ESP_OK);
  EXPECT_EQ(esp_intr_alloc(kCompatibilitySource, 0, InterruptHandler, &second,
                           &output),
            ESP_ERR_NOT_FOUND);
  EXPECT_EQ(output, sentinel);
  EXPECT_EQ(esp_intr_free(shared_handle), ESP_OK);
}

// Verifies same-source shared registrations preserve vector priority and IRAM
// compatibility, while allowing a request whose level mask includes the
// vector's selected level.
TEST(EspInterruptAllocator, EnforcesSharedVectorCompatibility) {
  HandlerState first;
  HandlerState second;
  intr_handle_t first_handle = nullptr;
  intr_handle_t compatible_handle = nullptr;
  constexpr int kFirstFlags =
      ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL2;
  ASSERT_EQ(esp_intr_alloc(kCompatibilitySource, kFirstFlags, InterruptHandler,
                           &first, &first_handle),
            ESP_OK);
  ASSERT_EQ(
      esp_intr_alloc(kCompatibilitySource, kFirstFlags | ESP_INTR_FLAG_LEVEL1,
                     InterruptHandler, &second, &compatible_handle),
      ESP_OK);

  intr_handle_t sentinel = reinterpret_cast<intr_handle_t>(uintptr_t{1});
  intr_handle_t output = sentinel;
  EXPECT_EQ(esp_intr_alloc(kCompatibilitySource,
                           ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_LEVEL2,
                           InterruptHandler, &second, &output),
            ESP_ERR_NOT_FOUND);
  EXPECT_EQ(output, sentinel);
  EXPECT_EQ(esp_intr_alloc(kCompatibilitySource,
                           ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_IRAM |
                               ESP_INTR_FLAG_LEVEL1,
                           InterruptHandler, &second, &output),
            ESP_ERR_NOT_FOUND);
  EXPECT_EQ(output, sentinel);

  EXPECT_EQ(esp_intr_free(first_handle), ESP_OK);
  EXPECT_EQ(esp_intr_free(compatible_handle), ESP_OK);
}

// Verifies native source assertions cannot cross a task-side free/reallocate
// boundary while fixed vector and registration slots are repeatedly reused.
TEST(EspInterruptAllocator, RacesNativeRaiseAgainstReuse) {
  HandlerState state;
  std::atomic<bool> keep_raising{true};
  portENTER_CRITICAL(nullptr);
  std::thread raiser([&keep_raising] {
    while (keep_raising.load(std::memory_order_acquire)) {
      roo_testing::esp_idf::raiseInterruptSource(kRaceSource);
      std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
  });
  portEXIT_CRITICAL(nullptr);

  bool lifecycle_ok = true;
  intr_handle_t live_handle = nullptr;
  for (int i = 0; i < 100; ++i) {
    const esp_err_t allocated =
        esp_intr_alloc(kRaceSource, 0, InterruptHandler, &state, &live_handle);
    if (allocated != ESP_OK) {
      ADD_FAILURE() << "allocation failed at iteration " << i << ": "
                    << allocated;
      lifecycle_ok = false;
      break;
    }
    const esp_err_t freed = esp_intr_free(live_handle);
    if (freed != ESP_OK) {
      ADD_FAILURE() << "free failed at iteration " << i << ": " << freed;
      lifecycle_ok = false;
      break;
    }
    live_handle = nullptr;
  }

  keep_raising.store(false, std::memory_order_release);
  raiser.join();
  if (live_handle != nullptr) {
    EXPECT_EQ(esp_intr_free(live_handle), ESP_OK);
  }
  EXPECT_TRUE(lifecycle_ok);

  const sig_atomic_t count_before_final_raise = state.count;
  intr_handle_t final_handle = nullptr;
  ASSERT_EQ(
      esp_intr_alloc(kRaceSource, 0, InterruptHandler, &state, &final_handle),
      ESP_OK);
  roo_testing::esp_idf::raiseInterruptSource(kRaceSource);
  EXPECT_TRUE(SpinUntil(&state.count, count_before_final_raise + 1));
  EXPECT_EQ(state.observed_isr, 1);
  EXPECT_EQ(esp_intr_free(final_handle), ESP_OK);
}

// Verifies upstream-invalid flag and status combinations are rejected without
// overwriting the caller's output handle.
TEST(EspInterruptAllocator, ValidatesAllocationArguments) {
  HandlerState state;
  intr_handle_t sentinel = reinterpret_cast<intr_handle_t>(uintptr_t{1});
  intr_handle_t output = sentinel;

  EXPECT_EQ(
      esp_intr_alloc(kTestSource, ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_EDGE,
                     InterruptHandler, &state, &output),
      ESP_ERR_INVALID_ARG);
  EXPECT_EQ(output, sentinel);
  EXPECT_EQ(esp_intr_alloc(kTestSource, ESP_INTR_FLAG_LEVEL4, InterruptHandler,
                           &state, &output),
            ESP_ERR_INVALID_ARG);
  EXPECT_EQ(output, sentinel);
  EXPECT_EQ(esp_intr_alloc_intrstatus(kTestSource, 0, kStatusAddressA, 0,
                                      InterruptHandler, &state, &output),
            ESP_ERR_INVALID_ARG);
  EXPECT_EQ(output, sentinel);
  EXPECT_EQ(
      esp_intr_alloc(ETS_INTERNAL_TIMER0_INTR_SOURCE, ESP_INTR_FLAG_SHARED,
                     InterruptHandler, &state, &output),
      ESP_ERR_INVALID_ARG);
  EXPECT_EQ(output, sentinel);

  intr_handle_t reserved = nullptr;
  ASSERT_EQ(esp_intr_alloc(kTestSource, ESP_INTR_FLAG_LEVEL4, nullptr, nullptr,
                           &reserved),
            ESP_OK);
  ASSERT_NE(reserved, nullptr);
  EXPECT_EQ(esp_intr_free(reserved), ESP_OK);
  EXPECT_EQ(esp_intr_free(nullptr), ESP_ERR_INVALID_ARG);
}
