#include "hardware/gpio.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "pico/time.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"
#include "roo_testing/transducers/voltage/voltage.h"

namespace {

using roo_testing_transducers::ConstantDuty;
using roo_testing_transducers::ConstVoltage;
using roo_testing_transducers::DigitalLevelFromVoltage;
using roo_testing_transducers::SimpleVoltageSink;
using roo_testing_transducers::SimpleVoltageSource;
using roo_testing_transducers::VoltageIO;
using roo_testing_transducers::VoltageSignal;

/// Exercises lazy board construction through real SDK calls before main.
bool ConfigureBootPin() {
  gpio_init(29);
  gpio_put(29, 1);
  gpio_set_dir(29, GPIO_OUT);
  return gpio_get(29) && gpio_get_function(29) == GPIO_FUNC_SIO;
}

const bool kBootPinWasHigh = ConfigureBootPin();

/// Models an external bidirectional device independently of the board's drive.
class ExternalIo : public VoltageIO {
 public:
  /// Returns the fixture's descriptive name.
  const std::string& name() const override { return name_; }

  /// Returns the external drive; NaN means the external device is not driving.
  float read() const override { return external_voltage_; }

  /// Records output signals without inventing an external driver.
  void write(const VoltageSignal& signal) override { signal_ = signal; }

  /// Selects the voltage driven by the external fixture.
  void setExternalVoltage(float value) { external_voltage_ = value; }

 private:
  std::string name_ = "pico_io";
  float external_voltage_ = std::nanf("");
  std::optional<VoltageSignal> signal_;
};

/// Keeps all borrowed wiring alive for the process-wide board.
struct Wiring {
  /// Attaches sources and observers before their pins are configured.
  Wiring()
      : output0(SimpleVoltageSink::WithSignalCallback(
            "gpio0", [this](const VoltageSignal& signal) { record(signal); })),
        feedback5(SimpleVoltageSink::WithSignalCallback(
            "gpio5",
            [this](const VoltageSignal& signal) {
              source6.set(DigitalLevelFromVoltage(
                  signal.voltageAtUptimeMicros(time_us_64())));
            })),
        source4("periodic", [this] {
          return waveform.voltageAtUptimeMicros(time_us_64());
        }) {
    FakePico().attachOutput(0, output0);
    FakePico().attachInput(1, source1);
    FakePico().attach(2, io2);
    FakePico().attachOutput(3, output3);
    FakePico().attachInput(4, source4);
    FakePico().attachOutput(5, feedback5);
    FakePico().attachInput(6, source6);
  }

  /// Observes coherent masked state and detects notifications from bad batches.
  void record(const VoltageSignal& signal) {
    if (forbid_notification) std::_Exit(77);
    events.push_back(signal.voltageAtUptimeMicros(time_us_64()));
    if (observe_batch) {
      batch_consistent =
          batch_consistent && gpio_get_out_level(0) == gpio_get_out_level(3);
    }
  }

  ConstVoltage source1{0.0f};
  ConstVoltage source6{0.0f};
  ExternalIo io2;
  SimpleVoltageSink output0;
  SimpleVoltageSink output3{"gpio3"};
  SimpleVoltageSink feedback5;
  VoltageSignal waveform = VoltageSignal::Constant(0);
  SimpleVoltageSource source4;
  std::vector<float> events;
  bool forbid_notification = false;
  bool observe_batch = false;
  bool batch_consistent = true;
};

/// Returns the fixed process-lifetime test topology.
Wiring& Wires() {
  static Wiring wiring;
  return wiring;
}

class PicoGpioTest : public testing::Test {
 protected:
  /// Restores pin configuration without detaching or shortening fixture
  /// lifetime.
  void SetUp() override {
    Wiring& wiring = Wires();
    wiring.source1.set(0.0f);
    wiring.source6.set(0.0f);
    wiring.io2.setExternalVoltage(std::nanf(""));
    wiring.waveform = VoltageSignal::Constant(0);
    wiring.forbid_notification = false;
    wiring.observe_batch = false;
    wiring.batch_consistent = true;
    gpio_init_mask(FakePicoBoard::kGpioMask);
    for (uint pin = 0; pin < FakePicoBoard::kGpioCount; ++pin)
      gpio_pull_down(pin);
    wiring.events.clear();
  }
};

using PicoGpioDeathTest = PicoGpioTest;

// Verifies invalid indices and high-bank bits fail before shifts or mutation.
TEST_F(PicoGpioDeathTest, RejectsInvalidPinsMasksAndBanks) {
  EXPECT_DEATH(gpio_put(30, 1), "Invalid Pico GPIO pin");
  EXPECT_DEATH(gpio_get(std::numeric_limits<uint>::max()),
               "Invalid Pico GPIO pin");
  EXPECT_DEATH(gpio_set_mask(uint32_t{1} << 30), "Invalid Pico GPIO mask");
  EXPECT_DEATH(gpio_put_masked64(uint64_t{1} << 63, 0),
               "Invalid Pico GPIO mask");
  EXPECT_DEATH(gpio_init_mask64(uint64_t{1} << 32), "Invalid Pico GPIO mask");
  EXPECT_DEATH(gpio_set_mask_n(1, 0), "Invalid Pico GPIO bank");
  EXPECT_DEATH(gpio_set_dir_all_bits64(uint64_t{1} << 40),
               "Invalid Pico GPIO mask");
}

// Verifies unavailable peripheral, pad, keeper, and IRQ modes are explicit
// errors.
TEST_F(PicoGpioDeathTest, RejectsUnsupportedConfigurations) {
  EXPECT_DEATH(gpio_set_function(7, GPIO_FUNC_PIO0),
               "Unsupported Pico GPIO function");
  EXPECT_DEATH(gpio_set_function(7, GPIO_FUNC_UART),
               "Unsupported Pico GPIO function");
  EXPECT_DEATH(gpio_set_function(7, static_cast<gpio_function_t>(255)),
               "Unsupported Pico GPIO function");
  EXPECT_DEATH(gpio_set_inover(7, 4), "Invalid Pico GPIO override");
  EXPECT_DEATH(gpio_set_pulls(7, true, true), "bus keeper");
  EXPECT_DEATH(gpio_set_irq_enabled(7, GPIO_IRQ_EDGE_RISE, false),
               "interrupts");
  EXPECT_DEATH(
      gpio_set_irq_enabled_with_callback(7, GPIO_IRQ_EDGE_RISE, true, nullptr),
      "interrupts");
  EXPECT_DEATH(gpio_add_raw_irq_handler_masked64(1, nullptr), "interrupts");
  EXPECT_DEATH(gpio_set_input_hysteresis_enabled(7, false),
               "analog pad controls");
  gpio_deinit(7);
  EXPECT_DEATH(gpio_set_oeover(7, GPIO_OVERRIDE_HIGH), "non-SIO");
}

// Verifies every mask member is checked before an observer sees any change.
TEST_F(PicoGpioDeathTest, RejectsWiringConflictsWithoutPartialPublication) {
  Wires().forbid_notification = true;
  EXPECT_DEATH(gpio_set_dir_out_masked(3), "output conflicts");
  EXPECT_DEATH(gpio_set_dir_out_masked(1u | (1u << 30)),
               "Invalid Pico GPIO mask");
  Wires().forbid_notification = false;
  EXPECT_FALSE(gpio_is_dir_out(0));
  EXPECT_FALSE(gpio_is_dir_out(1));
}

// Verifies duplicate and late wiring cannot silently replace a device.
TEST_F(PicoGpioDeathTest, RejectsDuplicateAndLateAttachments) {
  ConstVoltage source(0.0f);
  EXPECT_DEATH(FakePico().attachInput(1, source), "attachment conflict");
  EXPECT_DEATH(FakePico().attachInput(7, source), "before configuring");
  EXPECT_DEATH(FakePico().attachInput(30, source), "Invalid Pico GPIO pin");
}

// Verifies unknown pad voltage is rejected unless the digital path is forced.
TEST_F(PicoGpioDeathTest, RejectsFloatingAmbiguousAndOpposingDrivers) {
  gpio_disable_pulls(7);
  EXPECT_DEATH(gpio_get(7), "floating or undefined");
  Wires().source1.set(1.2f);
  EXPECT_DEATH(gpio_get(1), "floating or undefined");
  Wires().io2.setExternalVoltage(0.0f);
  gpio_put(2, 1);
  gpio_set_dir(2, GPIO_OUT);
  EXPECT_DEATH(gpio_get(2), "strong-driver contention");
}

// Verifies the process-wide board is usable during C++ static initialization.
TEST_F(PicoGpioTest, InitializesBeforeMainWithoutHardwareAccess) {
  EXPECT_TRUE(kBootPinWasHigh);
  EXPECT_EQ(&FakePico(), &FakePico());
}

// Verifies a software latch can be preloaded without driving an input source.
TEST_F(PicoGpioTest, SeparatesOutputLatchFromInputVoltage) {
  gpio_put(1, -2);
  EXPECT_TRUE(gpio_get_out_level(1));
  EXPECT_FALSE(gpio_get(1));
  EXPECT_EQ(gpio_get_dir(1), static_cast<uint>(GPIO_IN));
  EXPECT_FALSE(FakePico().driveSignal(1).has_value());
  EXPECT_TRUE(Wires().events.empty());
}

// Verifies output signals reach shared sinks and disappear from active drive on
// release.
TEST_F(PicoGpioTest, PublishesOutputAndReleasesWithoutInventingAVoltage) {
  gpio_put(0, 1);
  EXPECT_TRUE(Wires().events.empty());
  gpio_set_dir(0, GPIO_OUT);
  ASSERT_EQ(Wires().events.size(), size_t{1});
  EXPECT_FLOAT_EQ(Wires().events.back(), 3.3f);
  ASSERT_TRUE(FakePico().driveSignal(0).has_value());
  EXPECT_FLOAT_EQ(FakePico().driveSignal(0)->voltageAtUptimeMicros(123), 3.3f);
  gpio_set_dir(0, GPIO_IN);
  gpio_disable_pulls(0);
  EXPECT_FALSE(FakePico().driveSignal(0).has_value());
  EXPECT_TRUE(std::isnan(FakePico().padVoltage(0)));
  // Sink values are retained assignment history, not a high-impedance solver.
  EXPECT_FLOAT_EQ(Wires().output0.sampleAtUptimeMicros(123), 3.3f);
}

// Verifies pad-only changes and identical latch writes do not invent output
// edges.
TEST_F(PicoGpioTest, NotifiesOnlyWhenTheStrongOutputChanges) {
  gpio_set_dir(0, GPIO_OUT);
  gpio_put(0, 1);
  const size_t count = Wires().events.size();
  gpio_put(0, 1);
  gpio_pull_up(0);
  gpio_set_input_enabled(0, false);
  EXPECT_EQ(Wires().events.size(), count);
}

// Verifies weak pulls resolve unconnected inputs and yield to external drive.
TEST_F(PicoGpioTest, ModelsPullsAndInputEnable) {
  gpio_pull_up(7);
  EXPECT_TRUE(gpio_is_pulled_up(7));
  EXPECT_FALSE(gpio_is_pulled_down(7));
  EXPECT_TRUE(gpio_get(7));
  gpio_pull_down(7);
  EXPECT_FALSE(gpio_get(7));
  gpio_pull_up(1);
  EXPECT_FALSE(gpio_get(1));
  Wires().source1.set(3.3f);
  gpio_pull_down(1);
  EXPECT_TRUE(gpio_get(1));
  gpio_set_input_enabled(1, false);
  EXPECT_FALSE(gpio_get(1));
  gpio_set_inover(1, GPIO_OVERRIDE_INVERT);
  EXPECT_TRUE(gpio_get(1));
}

// Verifies input, output, and output-enable overrides remain separate from
// latches.
TEST_F(PicoGpioTest, AppliesOverridesWithoutChangingRawDirectionOrLatch) {
  gpio_put(0, 1);
  gpio_set_dir(0, GPIO_OUT);
  gpio_set_outover(0, GPIO_OVERRIDE_INVERT);
  EXPECT_TRUE(gpio_get_out_level(0));
  EXPECT_FALSE(gpio_get(0));
  gpio_set_outover(0, GPIO_OVERRIDE_HIGH);
  EXPECT_TRUE(gpio_get(0));
  gpio_set_oeover(0, GPIO_OVERRIDE_LOW);
  EXPECT_FALSE(FakePico().driveSignal(0).has_value());
  EXPECT_TRUE(gpio_is_dir_out(0));
  EXPECT_FALSE(gpio_get(0));
  gpio_set_dir(0, GPIO_IN);
  gpio_set_oeover(0, GPIO_OVERRIDE_HIGH);
  EXPECT_TRUE(FakePico().driveSignal(0).has_value());
  EXPECT_FALSE(gpio_is_dir_out(0));
  gpio_disable_pulls(7);
  gpio_set_inover(7, GPIO_OVERRIDE_HIGH);
  EXPECT_TRUE(gpio_get(7));
  gpio_set_inover(7, GPIO_OVERRIDE_LOW);
  EXPECT_FALSE(gpio_get(7));
}

// Verifies selecting a function clears overrides while retaining pulls and SIO
// registers.
TEST_F(PicoGpioTest, FollowsSdkInitDeinitAndFunctionSemantics) {
  gpio_pull_up(0);
  gpio_put(0, 1);
  gpio_set_dir(0, GPIO_OUT);
  gpio_set_inover(0, GPIO_OVERRIDE_LOW);
  gpio_set_outover(0, GPIO_OVERRIDE_LOW);
  gpio_set_oeover(0, GPIO_OVERRIDE_LOW);
  gpio_set_input_enabled(0, false);
  gpio_set_function(0, GPIO_FUNC_SIO);
  EXPECT_TRUE(gpio_get(0));
  EXPECT_TRUE(gpio_get_out_level(0));
  EXPECT_TRUE(gpio_is_dir_out(0));
  EXPECT_TRUE(gpio_is_pulled_up(0));
  gpio_deinit(0);
  EXPECT_EQ(gpio_get_function(0), GPIO_FUNC_NULL);
  EXPECT_FALSE(FakePico().driveSignal(0).has_value());
  EXPECT_TRUE(gpio_get(0));
  gpio_init(0);
  EXPECT_EQ(gpio_get_function(0), GPIO_FUNC_SIO);
  EXPECT_FALSE(gpio_is_dir_out(0));
  EXPECT_FALSE(gpio_get_out_level(0));
  EXPECT_TRUE(gpio_is_pulled_up(0));
}

// Verifies batch state is coherent before any selected output observer runs.
TEST_F(PicoGpioTest, UpdatesMasksCoherentlyAndSupportsLowBank64BitApis) {
  constexpr uint32_t mask = 1u | (1u << 3);
  gpio_set_dir_out_masked64(mask);
  Wires().observe_batch = true;
  gpio_set_mask64(mask);
  EXPECT_TRUE(Wires().batch_consistent);
  EXPECT_EQ(gpio_get_all64(), static_cast<uint64_t>(mask));
  gpio_xor_mask_n(0, mask);
  EXPECT_EQ(gpio_get_all(), 0u);
  gpio_put_masked64(mask, std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(gpio_get_all(), mask);
  gpio_clr_mask_n(0, mask);
  gpio_put_all(1u << 3);
  EXPECT_EQ(gpio_get_all(), 1u << 3);
  gpio_set_dir_all_bits64(0);
  EXPECT_EQ(gpio_get_all(), 0u);
  EXPECT_EQ(gpio_get_out_level(3), true);
  gpio_set_dir_masked64(mask, 1u << 3);
  EXPECT_TRUE(gpio_is_dir_out(3));
  EXPECT_FALSE(gpio_is_dir_out(0));
}

// Verifies retained voltage waveforms are sampled at shared manual uptime.
TEST_F(PicoGpioTest, ReadsPeriodicExternalSignalsAtTheSharedClock) {
  const int64_t start = static_cast<int64_t>(time_us_64());
  Wires().waveform =
      VoltageSignal::Square(0, 3.3f, 1000, ConstantDuty{0.5}, start);
  EXPECT_TRUE(gpio_get(4));
  sleep_us(500);
  EXPECT_FALSE(gpio_get(4));
  sleep_us(500);
  EXPECT_TRUE(gpio_get(4));
}

// Verifies an output observer can feed a shared voltage source on another
// input.
TEST_F(PicoGpioTest, ConnectsOutputToInputThroughSharedTransducers) {
  gpio_set_dir(5, GPIO_OUT);
  gpio_put(5, 1);
  EXPECT_TRUE(gpio_get(6));
  gpio_put(5, 0);
  EXPECT_FALSE(gpio_get(6));
}

}  // namespace
