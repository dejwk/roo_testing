#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_chip_info.h"
#include "gtest/gtest.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32_soc_gpio_struct.h"

#ifdef ARDUINO
#include "Arduino.h"
#endif

namespace {

// Verifies platform selection independently of the target macros under test.
TEST(SocPeripheralTest, ReportsSelectedChipAndControllerCounts) {
  esp_chip_info_t info{};
  esp_chip_info(&info);
#if ROO_EXPECTED_MODEL == 3
  EXPECT_EQ(CHIP_ESP32C3, info.model);
  EXPECT_EQ(1, info.cores);
  EXPECT_EQ(22, static_cast<int>(SOC_GPIO_PIN_COUNT));
  EXPECT_EQ(2, static_cast<int>(SOC_UART_NUM));
  EXPECT_EQ(1, static_cast<int>(SOC_I2C_NUM));
  EXPECT_EQ(2, static_cast<int>(SOC_SPI_PERIPH_NUM));
  EXPECT_EQ(6, static_cast<int>(SOC_LEDC_CHANNEL_NUM));
#elif ROO_EXPECTED_MODEL == 2
  EXPECT_EQ(CHIP_ESP32S3, info.model);
  EXPECT_EQ(2, info.cores);
  EXPECT_EQ(49, static_cast<int>(SOC_GPIO_PIN_COUNT));
  EXPECT_EQ(3, static_cast<int>(SOC_UART_NUM));
  EXPECT_EQ(2, static_cast<int>(SOC_I2C_NUM));
  EXPECT_EQ(3, static_cast<int>(SOC_SPI_PERIPH_NUM));
  EXPECT_EQ(8, static_cast<int>(SOC_LEDC_CHANNEL_NUM));
#else
  EXPECT_EQ(CHIP_ESP32, info.model);
  EXPECT_EQ(2, info.cores);
  EXPECT_EQ(40, static_cast<int>(SOC_GPIO_PIN_COUNT));
  EXPECT_EQ(3, static_cast<int>(SOC_UART_NUM));
  EXPECT_EQ(2, static_cast<int>(SOC_I2C_NUM));
  EXPECT_EQ(3, static_cast<int>(SOC_SPI_PERIPH_NUM));
#endif
#if !CONFIG_IDF_TARGET_ESP32
  EXPECT_EQ(0U, info.features & CHIP_FEATURE_BT);
#endif
  EXPECT_NE(0U, info.features & CHIP_FEATURE_WIFI_BGN);
  EXPECT_NE(0U, info.features & CHIP_FEATURE_BLE);
}

// Verifies missing pins and input-only outputs are rejected without access.
TEST(SocPeripheralTest, RejectsUnavailablePinsAndControllers) {
  EXPECT_EQ(ESP_ERR_INVALID_ARG,
            gpio_set_level(static_cast<gpio_num_t>(SOC_GPIO_PIN_COUNT), 1));
  EXPECT_EQ(ESP_ERR_INVALID_ARG,
            uart_driver_install(static_cast<uart_port_t>(SOC_UART_NUM), 64, 0,
                                0, nullptr, 0));
  gpio_config_t gpio{};
  gpio.mode = GPIO_MODE_OUTPUT;
  gpio.pin_bit_mask = uint64_t{1} << 4;
  EXPECT_EQ(ESP_OK, gpio_config(&gpio));
  gpio.pin_bit_mask = uint64_t{1} << SOC_GPIO_PIN_COUNT;
  EXPECT_EQ(ESP_ERR_INVALID_ARG, gpio_config(&gpio));
#ifdef ARDUINO
  EXPECT_FALSE(uartSetPins(0, -1, SOC_GPIO_PIN_COUNT, -1, -1));
#endif
  spi_bus_config_t spi{};
  EXPECT_EQ(
      ESP_ERR_INVALID_ARG,
      spi_bus_initialize(static_cast<spi_host_device_t>(SOC_SPI_PERIPH_NUM),
                         &spi, SPI_DMA_DISABLED));
  i2c_master_bus_config_t i2c{};
  i2c.i2c_port = SOC_I2C_NUM;
  i2c.sda_io_num = GPIO_NUM_4;
  i2c.scl_io_num = GPIO_NUM_5;
  i2c_master_bus_handle_t bus = nullptr;
  EXPECT_EQ(ESP_ERR_INVALID_ARG, i2c_new_master_bus(&i2c, &bus));
  EXPECT_EQ(ESP_ERR_INVALID_ARG,
            i2c_reset_tx_fifo(static_cast<i2c_port_t>(SOC_I2C_NUM)));
#if CONFIG_IDF_TARGET_ESP32S3
  EXPECT_EQ(ESP_ERR_INVALID_ARG,
            gpio_set_level(static_cast<gpio_num_t>(22), 1));
#elif CONFIG_IDF_TARGET_ESP32
  EXPECT_EQ(ESP_ERR_INVALID_ARG,
            gpio_set_level(static_cast<gpio_num_t>(34), 1));
#endif
}

// Verifies raw register storage uses the selected chip's address window.
TEST(SocPeripheralTest, StoresRegisterMasksInTheTargetAddressWindow) {
  FakeEsp32().reg().write(0x60000000, 0xFFFF, 0x1234);
  FakeEsp32().reg().write(0x60000000, 0xFF00, 0xAB00);
  EXPECT_EQ(0xAB34U, FakeEsp32().reg().read(0x60000000, 0xFFFF));
}

// Verifies C3-style .val access still drives and samples the host GPIO model.
TEST(SocPeripheralTest, InterceptsUnionStyleLowBankRegisters) {
  constexpr int kPin = 4;
  GPIO.out_w1ts.val = 1U << kPin;
  EXPECT_NE(0U, static_cast<uint32_t>(GPIO.in.val) & (1U << kPin));
  GPIO.out_w1tc.val = 1U << kPin;
  EXPECT_EQ(0U, static_cast<uint32_t>(GPIO.in.val) & (1U << kPin));
}

// Verifies a real S3 GPIO48 cannot alias a detached matrix input.
TEST(SocPeripheralTest, SeparatesPinsFromMatrixSentinels) {
#if CONFIG_IDF_TARGET_ESP32S3
  constexpr int kPin = 48;
#else
  constexpr int kPin = 4;
#endif
#ifdef ARDUINO
  EXPECT_EQ(TX, roo_testing::soc::kConsoleTx);
  EXPECT_EQ(RX, roo_testing::soc::kConsoleRx);
#if ROO_EXPECTED_MODEL == 3
  EXPECT_STREQ("ESP32-C3", ESP.getChipModel());
  EXPECT_EQ(160U, getCpuFrequencyMhz());
#elif ROO_EXPECTED_MODEL == 2
  EXPECT_STREQ("ESP32-S3", ESP.getChipModel());
  EXPECT_EQ(240U, getCpuFrequencyMhz());
#else
  EXPECT_STREQ("ESP32", ESP.getChipModel());
  EXPECT_EQ(240U, getCpuFrequencyMhz());
#endif
#endif
  EXPECT_EQ(GPIO_MATRIX_CONST_ZERO_INPUT, kMatrixDetachInLowPin);
  EXPECT_EQ(GPIO_MATRIX_CONST_ONE_INPUT, kMatrixDetachInHighPin);
  const uint8_t signal = roo_testing::soc::kUartRxSignals[0];
  FakeEsp32().in_matrix.assign(kPin, signal, false);
  EXPECT_EQ(kPin, FakeEsp32().in_matrix.pin_for_signal(signal));
  FakeEsp32().in_matrix.assign(kMatrixDetachInLowPin, signal, false);
  EXPECT_EQ(kMatrixDetachInLowPin,
            FakeEsp32().in_matrix.pin_for_signal(signal));
  FakeEsp32().in_matrix.assign(kMatrixDetachInHighPin, signal, false);
  EXPECT_EQ(kMatrixDetachInHighPin,
            FakeEsp32().in_matrix.pin_for_signal(signal));
  FakeEsp32().in_matrix.assign(kMatrixDetachInUndefPin, signal, false);
  EXPECT_EQ(kMatrixDetachInUndefPin,
            FakeEsp32().in_matrix.pin_for_signal(signal));
#if CONFIG_IDF_TARGET_ESP32S3
  GPIO.out1_w1ts.val = 1U << 16;
  EXPECT_NE(0U, static_cast<uint32_t>(GPIO.in1.val) & (1U << 16));
  EXPECT_GT(FakeEsp32().gpio.get(48).last_written(), 3.0f);
  GPIO.out1_w1tc.val = 1U << 16;
  EXPECT_EQ(0U, static_cast<uint32_t>(GPIO.in1.val) & (1U << 16));
  EXPECT_FLOAT_EQ(0.0f, FakeEsp32().gpio.get(48).last_written());
#endif
}

// Verifies ADC routing follows each chip's public channel map, including ADC2.
TEST(SocPeripheralTest, ConvertsTheSelectedAdcPins) {
  for (int unit = 0; unit < 2; ++unit) {
    for (int channel = 0; channel < 10; ++channel) {
      const int pin = roo_testing::soc::kAdcPins[unit][channel];
      if (pin < 0) continue;
      roo_testing_transducers::ConstVoltage source("ADC input", 0.5f);
      FakeEsp32().gpio.attachInput(pin, source);
      FakeEsp32().adc(unit).setAttenuation(channel, 0);
      EXPECT_NEAR(1846, FakeEsp32().adc(unit).convert(channel), 10);
#ifdef ARDUINO
      EXPECT_EQ(unit * 10 + channel, digitalPinToAnalogChannel(pin));
      EXPECT_EQ(pin, analogChannelToDigitalPin(unit * 10 + channel));
#endif
      FakeEsp32().gpio.detach(pin);
    }
  }
#ifdef ARDUINO
  EXPECT_EQ(-1, digitalPinToAnalogChannel(255));
  EXPECT_EQ(-1, analogChannelToDigitalPin(255));
#endif
}

// Verifies a chip's maximum LEDC channel and resolution are enforced.
TEST(SocPeripheralTest, EnforcesLedcLimits) {
  ledc_timer_config_t timer{};
  timer.speed_mode = LEDC_LOW_SPEED_MODE;
  timer.timer_num = LEDC_TIMER_0;
  timer.duty_resolution = LEDC_TIMER_8_BIT;
  timer.freq_hz = 1000;
  ASSERT_EQ(ESP_OK, ledc_timer_config(&timer));
  ledc_channel_config_t channel{};
  channel.speed_mode = LEDC_LOW_SPEED_MODE;
  channel.channel = static_cast<ledc_channel_t>(SOC_LEDC_CHANNEL_NUM - 1);
  channel.gpio_num = 4;
  channel.timer_sel = LEDC_TIMER_0;
  ASSERT_EQ(ESP_OK, ledc_channel_config(&channel));
  channel.channel = static_cast<ledc_channel_t>(SOC_LEDC_CHANNEL_NUM);
  EXPECT_EQ(ESP_ERR_INVALID_ARG, ledc_channel_config(&channel));
#ifdef ARDUINO
  EXPECT_FALSE(ledcAttachChannel(4, 1000, SOC_LEDC_TIMER_BIT_WIDTH + 1, 0));
#endif
}

}  // namespace
