#include <gtest/gtest.h>

#include "driver/ledc.h"

namespace {

TEST(IdfLedcTest, ConfiguresAndUpdatesDutyAndFadeState) {
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

  EXPECT_EQ(ESP_OK,
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 256));
  EXPECT_EQ(64U, ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
  EXPECT_EQ(ESP_OK,
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
  EXPECT_EQ(256U, ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

  EXPECT_EQ(ESP_OK, ledc_fade_func_install(0));
  EXPECT_EQ(ESP_OK, ledc_set_fade_with_time(
                        LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 768, 100));
  EXPECT_EQ(ESP_OK, ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0,
                                    LEDC_FADE_NO_WAIT));
  EXPECT_EQ(768U, ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

TEST(IdfLedcTest, RejectsInvalidArgumentsAndUnconfiguredChannels) {
  EXPECT_EQ(ESP_ERR_INVALID_ARG, ledc_timer_config(nullptr));
  EXPECT_EQ(ESP_ERR_INVALID_ARG, ledc_channel_config(nullptr));
  EXPECT_EQ(ESP_ERR_INVALID_STATE,
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 1));
  EXPECT_EQ(LEDC_ERR_DUTY,
            ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
}

}  // namespace
