#include "examples/pico_gpio/blink.h"

#include <array>
#include <cstdint>
#include <vector>

#include "gtest/gtest.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"
#include "roo_testing/transducers/voltage/voltage.h"

namespace {

struct Sample {
  uint64_t uptime_us;
  float voltage;
};

std::vector<Sample> kSamples;

/// Records waveform-aware LED assignments at the same uptime as SDK sleeps.
void RecordLed(const roo_testing_transducers::VoltageSignal& signal) {
  const uint64_t now_us = time_us_64();
  kSamples.push_back({now_us, signal.voltageAtUptimeMicros(now_us)});
}

roo_testing_transducers::SimpleVoltageSink kLed =
    roo_testing_transducers::SimpleVoltageSink::WithSignalCallback("Pico LED",
                                                                   RecordLed);

// Verifies the bounded sketch produces six exact edges plus its initial low
// drive.
TEST(PicoBlinkTest, EmitsThreeCyclesAtManualUptime) {
  FakePico().attachOutput(PICO_DEFAULT_LED_PIN, kLed);
  const uint64_t start = time_us_64();
  RunPicoBlink();
  const std::array<uint64_t, 7> offsets{0, 0, 1000, 2000, 3000, 4000, 5000};
  const std::array<float, 7> levels{0, 3.3f, 0, 3.3f, 0, 3.3f, 0};
  ASSERT_EQ(kSamples.size(), offsets.size());
  for (size_t i = 0; i < offsets.size(); ++i) {
    EXPECT_EQ(kSamples[i].uptime_us, start + offsets[i]);
    EXPECT_FLOAT_EQ(kSamples[i].voltage, levels[i]);
  }
  EXPECT_EQ(time_us_64(), start + 6000);
  EXPECT_FALSE(gpio_get_out_level(PICO_DEFAULT_LED_PIN));
}

}  // namespace
