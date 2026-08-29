#include <gtest/gtest.h>

#include "driver/ledc.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/transducers/voltage/voltage_signal.h"

namespace {

TEST(IdfLedcTest, PublishesCommittedDutyOnly) {
  ledc_timer_config_t timer{};
  timer.speed_mode = LEDC_LOW_SPEED_MODE;
  timer.duty_resolution = LEDC_TIMER_10_BIT;
  timer.timer_num = LEDC_TIMER_0;
  timer.freq_hz = 5000;
  timer.clk_cfg = LEDC_AUTO_CLK;
  EXPECT_EQ(ESP_OK, ledc_timer_config(&timer));

  ledc_channel_config_t channel{};
  channel.gpio_num = 18;
  channel.speed_mode = LEDC_LOW_SPEED_MODE;
  channel.channel = LEDC_CHANNEL_0;
  channel.timer_sel = LEDC_TIMER_0;
  channel.duty = 64;
  EXPECT_EQ(ESP_OK, ledc_channel_config(&channel));
  EXPECT_EQ(64U, ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

  EXPECT_EQ(ESP_OK, ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 256));
  EXPECT_EQ(64U, ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
  EXPECT_EQ(ESP_OK, ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
  EXPECT_EQ(256U, ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

  const auto signal = FakeEsp32().gpio.get(18).lastSignal();
  ASSERT_TRUE(signal.has_value());
  ASSERT_EQ(roo_testing_transducers::VoltageSignalKind::kSquare,
            signal->kind());
  const auto& square =
      std::get<roo_testing_transducers::SquareVoltageSpec>(signal->spec());
  EXPECT_DOUBLE_EQ(5000.0, square.carrier.frequency_hz);
  EXPECT_DOUBLE_EQ(
      0.25, std::get<roo_testing_transducers::ConstantDuty>(square.duty).duty);
  EXPECT_DOUBLE_EQ(0.0, square.pulse_start_phase_cycles);
  EXPECT_FALSE(square.inverted);
}

TEST(IdfLedcTest, PublishesPhaseAndInversionAndReconfiguration) {
  ledc_timer_config_t timer{};
  timer.speed_mode = LEDC_LOW_SPEED_MODE;
  timer.duty_resolution = LEDC_TIMER_4_BIT;
  timer.timer_num = LEDC_TIMER_1;
  timer.freq_hz = 1000;
  timer.clk_cfg = LEDC_AUTO_CLK;
  ASSERT_EQ(ESP_OK, ledc_timer_config(&timer));

  ledc_channel_config_t channel{};
  channel.gpio_num = 19;
  channel.speed_mode = LEDC_LOW_SPEED_MODE;
  channel.channel = LEDC_CHANNEL_1;
  channel.timer_sel = LEDC_TIMER_1;
  channel.duty = 8;
  channel.hpoint = 4;
  channel.flags.output_invert = true;
  ASSERT_EQ(ESP_OK, ledc_channel_config(&channel));

  auto signal = FakeEsp32().gpio.get(19).lastSignal();
  ASSERT_TRUE(signal.has_value());
  auto square =
      std::get<roo_testing_transducers::SquareVoltageSpec>(signal->spec());
  EXPECT_DOUBLE_EQ(
      0.5, std::get<roo_testing_transducers::ConstantDuty>(square.duty).duty);
  EXPECT_DOUBLE_EQ(0.25, square.pulse_start_phase_cycles);
  EXPECT_TRUE(square.inverted);

  timer.freq_hz = 2000;
  timer.duty_resolution = LEDC_TIMER_3_BIT;
  ASSERT_EQ(ESP_OK, ledc_timer_config(&timer));
  EXPECT_EQ(8U, ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
  signal = FakeEsp32().gpio.get(19).lastSignal();
  ASSERT_TRUE(signal.has_value());
  square = std::get<roo_testing_transducers::SquareVoltageSpec>(signal->spec());
  EXPECT_DOUBLE_EQ(2000.0, square.carrier.frequency_hz);
  EXPECT_DOUBLE_EQ(
      1.0, std::get<roo_testing_transducers::ConstantDuty>(square.duty).duty);
  EXPECT_DOUBLE_EQ(0.5, square.pulse_start_phase_cycles);
}

TEST(IdfLedcTest, MovesAndDeconfiguresOutputsWithoutLeavingOldPinDriven) {
  ledc_timer_config_t timer{};
  timer.speed_mode = LEDC_LOW_SPEED_MODE;
  timer.duty_resolution = LEDC_TIMER_3_BIT;
  timer.timer_num = LEDC_TIMER_2;
  timer.freq_hz = 1000;
  ASSERT_EQ(ESP_OK, ledc_timer_config(&timer));

  ledc_channel_config_t channel{};
  channel.gpio_num = 20;
  channel.speed_mode = LEDC_LOW_SPEED_MODE;
  channel.channel = LEDC_CHANNEL_2;
  channel.timer_sel = LEDC_TIMER_2;
  channel.duty = 4;
  ASSERT_EQ(ESP_OK, ledc_channel_config(&channel));
  channel.gpio_num = 21;
  ASSERT_EQ(ESP_OK, ledc_channel_config(&channel));

  const auto old_signal = FakeEsp32().gpio.get(20).lastSignal();
  ASSERT_TRUE(old_signal.has_value());
  EXPECT_EQ(roo_testing_transducers::VoltageSignalKind::kConstant,
            old_signal->kind());
  EXPECT_FLOAT_EQ(0.0f, old_signal->voltageAtUptimeMicros(0));

  channel.deconfigure = true;
  ASSERT_EQ(ESP_OK, ledc_channel_config(&channel));
  const auto new_signal = FakeEsp32().gpio.get(21).lastSignal();
  ASSERT_TRUE(new_signal.has_value());
  EXPECT_EQ(roo_testing_transducers::VoltageSignalKind::kConstant,
            new_signal->kind());
}

TEST(IdfLedcTest, RejectsInvalidArgumentsAndUnconfiguredChannels) {
  EXPECT_EQ(ESP_ERR_INVALID_ARG, ledc_timer_config(nullptr));
  EXPECT_EQ(ESP_ERR_INVALID_ARG, ledc_channel_config(nullptr));
  EXPECT_EQ(ESP_ERR_INVALID_STATE,
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_4, 1));
  EXPECT_EQ(LEDC_ERR_DUTY, ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_4));
}

TEST(IdfLedcTest, UnsupportedFadesLeaveCommittedOutputUntouched) {
  ledc_timer_config_t timer{};
  timer.speed_mode = LEDC_LOW_SPEED_MODE;
  timer.duty_resolution = LEDC_TIMER_2_BIT;
  timer.timer_num = LEDC_TIMER_3;
  timer.freq_hz = 1000;
  ASSERT_EQ(ESP_OK, ledc_timer_config(&timer));
  ledc_channel_config_t channel{};
  channel.gpio_num = 22;
  channel.speed_mode = LEDC_LOW_SPEED_MODE;
  channel.channel = LEDC_CHANNEL_3;
  channel.timer_sel = LEDC_TIMER_3;
  channel.duty = 2;
  ASSERT_EQ(ESP_OK, ledc_channel_config(&channel));

  EXPECT_EQ(ESP_ERR_NOT_SUPPORTED, ledc_fade_func_install(0));
  EXPECT_EQ(
      ESP_ERR_NOT_SUPPORTED,
      ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, 4, 10));
  EXPECT_EQ(
      ESP_ERR_NOT_SUPPORTED,
      ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, LEDC_FADE_NO_WAIT));
  EXPECT_EQ(2U, ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3));
}

}  // namespace
