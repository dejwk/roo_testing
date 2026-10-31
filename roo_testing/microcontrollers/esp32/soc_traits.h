#pragma once

#include <array>
#include <cstdint>

#include "esp_bit_defs.h"
#include "soc/adc_channel.h"
#include "soc/gpio_pins.h"
#include "soc/gpio_sig_map.h"
#include "soc/soc_caps.h"
#include "soc/uart_pins.h"

namespace roo_testing::soc {

/// Tests whether a physical GPIO exists on the selected chip.
constexpr bool IsValidPin(int pin) {
  return pin >= 0 && pin < SOC_GPIO_PIN_COUNT &&
         ((uint64_t{SOC_GPIO_VALID_GPIO_MASK} >> pin) & 1) != 0;
}

/// Tests whether a GPIO supports output on the selected chip.
constexpr bool IsOutputPin(int pin) {
  return IsValidPin(pin) &&
         ((uint64_t{SOC_GPIO_VALID_OUTPUT_GPIO_MASK} >> pin) & 1) != 0;
}

inline constexpr std::array<uint8_t, SOC_UART_NUM> kUartTxSignals = {
    U0TXD_OUT_IDX,
    U1TXD_OUT_IDX,
#if SOC_UART_NUM > 2
    U2TXD_OUT_IDX,
#endif
};
inline constexpr std::array<uint8_t, SOC_UART_NUM> kUartRxSignals = {
    U0RXD_IN_IDX,
    U1RXD_IN_IDX,
#if SOC_UART_NUM > 2
    U2RXD_IN_IDX,
#endif
};
inline constexpr std::array<uint8_t, SOC_I2C_NUM> kI2cSdaSignals = {
    I2CEXT0_SDA_OUT_IDX,
#if SOC_I2C_NUM > 1
    I2CEXT1_SDA_OUT_IDX,
#endif
};
inline constexpr std::array<uint8_t, SOC_I2C_NUM> kI2cSclSignals = {
    I2CEXT0_SCL_OUT_IDX,
#if SOC_I2C_NUM > 1
    I2CEXT1_SCL_OUT_IDX,
#endif
};

// Indexed by IDF host number: internal flash, SPI2, and (where present) SPI3.
#if CONFIG_IDF_TARGET_ESP32
inline constexpr std::array<uint8_t, SOC_SPI_PERIPH_NUM> kSpiClockSignals = {
    SPICLK_OUT_IDX, HSPICLK_OUT_IDX, VSPICLK_OUT_IDX};
inline constexpr std::array<uint8_t, SOC_SPI_PERIPH_NUM> kSpiMisoSignals = {
    SPIQ_IN_IDX, HSPIQ_IN_IDX, VSPIQ_IN_IDX};
inline constexpr std::array<uint8_t, SOC_SPI_PERIPH_NUM> kSpiMosiSignals = {
    SPID_OUT_IDX, HSPID_OUT_IDX, VSPID_OUT_IDX};
#else
inline constexpr std::array<uint8_t, SOC_SPI_PERIPH_NUM> kSpiClockSignals = {
    SPICLK_OUT_IDX,
    FSPICLK_OUT_IDX,
#if CONFIG_IDF_TARGET_ESP32S3
    SPI3_CLK_OUT_IDX,
#endif
};
inline constexpr std::array<uint8_t, SOC_SPI_PERIPH_NUM> kSpiMisoSignals = {
    SPIQ_IN_IDX,
    FSPIQ_IN_IDX,
#if CONFIG_IDF_TARGET_ESP32S3
    SPI3_Q_IN_IDX,
#endif
};
inline constexpr std::array<uint8_t, SOC_SPI_PERIPH_NUM> kSpiMosiSignals = {
    SPID_OUT_IDX,
    FSPID_OUT_IDX,
#if CONFIG_IDF_TARGET_ESP32S3
    SPI3_D_OUT_IDX,
#endif
};
#endif

inline constexpr int kConsoleTx = U0TXD_GPIO_NUM;
inline constexpr int kConsoleRx = U0RXD_GPIO_NUM;

#if CONFIG_IDF_TARGET_ESP32S3
inline constexpr char kChipModel[] = "ESP32-S3";
#elif CONFIG_IDF_TARGET_ESP32C3
inline constexpr char kChipModel[] = "ESP32-C3";
#else
inline constexpr char kChipModel[] = "ESP32";
#endif

// Ten slots per ADC preserve Arduino's unit * 10 + channel numbering.
inline constexpr std::array<std::array<int8_t, 10>, 2> kAdcPins = {{
#if CONFIG_IDF_TARGET_ESP32
    {ADC1_CHANNEL_0_GPIO_NUM, ADC1_CHANNEL_1_GPIO_NUM, ADC1_CHANNEL_2_GPIO_NUM,
     ADC1_CHANNEL_3_GPIO_NUM, ADC1_CHANNEL_4_GPIO_NUM, ADC1_CHANNEL_5_GPIO_NUM,
     ADC1_CHANNEL_6_GPIO_NUM, ADC1_CHANNEL_7_GPIO_NUM, -1, -1},
    {ADC2_CHANNEL_0_GPIO_NUM, ADC2_CHANNEL_1_GPIO_NUM, ADC2_CHANNEL_2_GPIO_NUM,
     ADC2_CHANNEL_3_GPIO_NUM, ADC2_CHANNEL_4_GPIO_NUM, ADC2_CHANNEL_5_GPIO_NUM,
     ADC2_CHANNEL_6_GPIO_NUM, ADC2_CHANNEL_7_GPIO_NUM, ADC2_CHANNEL_8_GPIO_NUM,
     ADC2_CHANNEL_9_GPIO_NUM},
#elif CONFIG_IDF_TARGET_ESP32S3
    {ADC1_CHANNEL_0_GPIO_NUM, ADC1_CHANNEL_1_GPIO_NUM, ADC1_CHANNEL_2_GPIO_NUM,
     ADC1_CHANNEL_3_GPIO_NUM, ADC1_CHANNEL_4_GPIO_NUM, ADC1_CHANNEL_5_GPIO_NUM,
     ADC1_CHANNEL_6_GPIO_NUM, ADC1_CHANNEL_7_GPIO_NUM, ADC1_CHANNEL_8_GPIO_NUM,
     ADC1_CHANNEL_9_GPIO_NUM},
    {ADC2_CHANNEL_0_GPIO_NUM, ADC2_CHANNEL_1_GPIO_NUM, ADC2_CHANNEL_2_GPIO_NUM,
     ADC2_CHANNEL_3_GPIO_NUM, ADC2_CHANNEL_4_GPIO_NUM, ADC2_CHANNEL_5_GPIO_NUM,
     ADC2_CHANNEL_6_GPIO_NUM, ADC2_CHANNEL_7_GPIO_NUM, ADC2_CHANNEL_8_GPIO_NUM,
     ADC2_CHANNEL_9_GPIO_NUM},
#elif CONFIG_IDF_TARGET_ESP32C3
    {ADC1_CHANNEL_0_GPIO_NUM, ADC1_CHANNEL_1_GPIO_NUM, ADC1_CHANNEL_2_GPIO_NUM,
     ADC1_CHANNEL_3_GPIO_NUM, ADC1_CHANNEL_4_GPIO_NUM, -1, -1, -1, -1, -1},
    {ADC2_CHANNEL_0_GPIO_NUM, -1, -1, -1, -1, -1, -1, -1, -1, -1},
#endif
}};

}  // namespace roo_testing::soc
