#include "fake_esp32.h"

#include <limits.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <set>
#include <thread>

#include "esp32-hal-spi.h"
#include "glog/logging.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_struct.h"
#include "soc/spi_struct.h"

FakeEsp32Board& FakeEsp32() {
  static FakeEsp32Board esp32;
  return esp32;
}

FakeGpioInterface* getGpioInterface() { return &FakeEsp32().gpio; }

FakeI2cInterface* getI2cInterface(uint8_t i2c_num) {
  return &FakeEsp32().i2c[i2c_num];
}

gpio_dev_t GPIO;

namespace {

static const char* in_id[] = {
    "SPICLK_IN",
    "SPIQ_IN",
    "SPID_IN",
    "SPIHD_IN",
    "SPIWP_IN",
    "SPICS0_IN",
    "SPICS1_IN",
    "SPICS2_IN",
    "HSPICLK_IN",
    "HSPIQ_IN",
    "HSPID_IN",
    "HSPICS0_IN",
    "HSPIHD_IN",
    "HSPIWP_IN",
    "U0RXD_IN",
    "U0CTS_IN",
    "U0DSR_IN",
    "U1RXD_IN",
    "U1CTS_IN",
    "I2CM_SCL_O",
    "I2CM_SDA_I",
    "EXT_I2C_SCL_O",
    "EXT_I2C_SDA_I",
    "I2S0O_BCK_IN",
    "I2S1O_BCK_IN",
    "I2S0O_WS_IN",
    "I2S1O_WS_IN",
    "I2S0I_BCK_IN",
    "I2S0I_WS_IN",
    "I2CEXT0_SCL_IN",
    "I2CEXT0_SDA_IN",
    "PWM0_SYNC0_IN",
    "PWM0_SYNC1_IN",
    "PWM0_SYNC2_IN",
    "PWM0_F0_IN",
    "PWM0_F1_IN",
    "PWM0_F2_IN",
    "GPIO_BT_ACTIVE",
    "GPIO_BT_PRIORITY",
    "PCNT_SIG_CH0_IN0",
    "PCNT_SIG_CH1_IN0",
    "PCNT_CTRL_CH0_IN0",
    "PCNT_CTRL_CH1_IN0",
    "PCNT_SIG_CH0_IN1",
    "PCNT_SIG_CH1_IN1",
    "PCNT_CTRL_CH0_IN1",
    "PCNT_CTRL_CH1_IN1",
    "PCNT_SIG_CH0_IN2",
    "PCNT_SIG_CH1_IN2",
    "PCNT_CTRL_CH0_IN2",
    "PCNT_CTRL_CH1_IN2",
    "PCNT_SIG_CH0_IN3",
    "PCNT_SIG_CH1_IN3",
    "PCNT_CTRL_CH0_IN3",
    "PCNT_CTRL_CH1_IN3",
    "PCNT_SIG_CH0_IN4",
    "PCNT_SIG_CH1_IN4",
    "PCNT_CTRL_CH0_IN4",
    "PCNT_CTRL_CH1_IN4",
    "BB_DIAG18",
    "BB_DIAG19",
    "HSPICS1_IN",
    "HSPICS2_IN",
    "VSPICLK_IN",
    "VSPIQ_IN",
    "VSPID_IN",
    "VSPIHD_IN",
    "VSPIWP_IN",
    "VSPICS0_IN",
    "VSPICS1_IN",
    "VSPICS2_IN",
    "PCNT_SIG_CH0_IN5",
    "PCNT_SIG_CH1_IN5",
    "PCNT_CTRL_CH0_IN5",
    "PCNT_CTRL_CH1_IN5",
    "PCNT_SIG_CH0_IN6",
    "PCNT_SIG_CH1_IN6",
    "PCNT_CTRL_CH0_IN6",
    "PCNT_CTRL_CH1_IN6",
    "PCNT_SIG_CH0_IN7",
    "PCNT_SIG_CH1_IN7",
    "PCNT_CTRL_CH0_IN7",
    "PCNT_CTRL_CH1_IN7",
    "RMT_SIG_IN0",
    "RMT_SIG_IN1",
    "RMT_SIG_IN2",
    "RMT_SIG_IN3",
    "RMT_SIG_IN4",
    "RMT_SIG_IN5",
    "RMT_SIG_IN6",
    "RMT_SIG_IN7",
    "UNDEF",
    "UNDEF",
    "EXT_ADC_START",
    "CAN_RX",
    "I2CEXT1_SCL_IN",
    "I2CEXT1_SDA_IN",
    "HOST_CARD_DETECT_N_1",
    "HOST_CARD_DETECT_N_2",
    "HOST_CARD_WRITE_PRT_1",
    "HOST_CARD_WRITE_PRT_2",
    "HOST_CARD_INT_N_1",
    "HOST_CARD_INT_N_2",
    "PWM1_SYNC0_IN",
    "PWM1_SYNC1_IN",
    "PWM1_SYNC2_IN",
    "PWM1_F0_IN",
    "PWM1_F1_IN",
    "PWM1_F2_IN",
    "PWM0_CAP0_IN",
    "PWM0_CAP1_IN",
    "PWM0_CAP2_IN",
    "PWM1_CAP0_IN",
    "PWM1_CAP1_IN",
    "PWM1_CAP2_IN",
    "PWM2_FLTA",
    "PWM2_FLTB",
    "PWM2_CAP1_IN",
    "PWM2_CAP2_IN",
    "PWM2_CAP3_IN",
    "PWM3_FLTA",
    "PWM3_FLTB",
    "PWM3_CAP1_IN",
    "PWM3_CAP2_IN",
    "PWM3_CAP3_IN",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "SPID4_IN",
    "SPID5_IN",
    "SPID6_IN",
    "SPID7_IN",
    "HSPID4_IN",
    "HSPID5_IN",
    "HSPID6_IN",
    "HSPID7_IN",
    "VSPID4_IN",
    "VSPID5_IN",
    "VSPID6_IN",
    "VSPID7_IN",
    "I2S0I_DATA_IN0",
    "I2S0I_DATA_IN1",
    "I2S0I_DATA_IN2",
    "I2S0I_DATA_IN3",
    "I2S0I_DATA_IN4",
    "I2S0I_DATA_IN5",
    "I2S0I_DATA_IN6",
    "I2S0I_DATA_IN7",
    "I2S0I_DATA_IN8",
    "I2S0I_DATA_IN9",
    "I2S0I_DATA_IN10",
    "I2S0I_DATA_IN11",
    "I2S0I_DATA_IN12",
    "I2S0I_DATA_IN13",
    "I2S0I_DATA_IN14",
    "I2S0I_DATA_IN15",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "I2S1I_BCK_IN",
    "I2S1I_WS_IN",
    "I2S1I_DATA_IN0",
    "I2S1I_DATA_IN1",
    "I2S1I_DATA_IN2",
    "I2S1I_DATA_IN3",
    "I2S1I_DATA_IN4",
    "I2S1I_DATA_IN5",
    "I2S1I_DATA_IN6",
    "I2S1I_DATA_IN7",
    "I2S1I_DATA_IN8",
    "I2S1I_DATA_IN9",
    "I2S1I_DATA_IN10",
    "I2S1I_DATA_IN11",
    "I2S1I_DATA_IN12",
    "I2S1I_DATA_IN13",
    "I2S1I_DATA_IN14",
    "I2S1I_DATA_IN15",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "I2S0I_H_SYNC",
    "I2S0I_V_SYNC",
    "I2S0I_H_ENABLE",
    "I2S1I_H_SYNC",
    "I2S1I_V_SYNC",
    "I2S1I_H_ENABLE",
    "UNDEF",
    "UNDEF",
    "U2RXD_IN",
    "U2CTS_IN",
    "EMAC_MDC_I",
    "EMAC_MDI_I",
    "EMAC_CRS_I",
    "EMAC_COL_I",
    "PCMFSYNC_IN",
    "PCMCLK_IN",
    "PCMDIN",
    "BLE_AUDIO0_IRQ",
    "BLE_AUDIO1_IRQ",
    "BLE_AUDIO2_IRQ",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "BLE_AUDIO_SYNC0_P",
    "BLE_AUDIO_SYNC1_P",
    "BLE_AUDIO_SYNC2_P",
    "ANT_SEL0",
    "ANT_SEL1",
    "ANT_SEL2",
    "ANT_SEL3",
    "ANT_SEL4",
    "ANT_SEL5",
    "ANT_SEL6",
    "ANT_SEL7",
    "SIG_IN_FUNC224",
    "SIG_IN_FUNC225",
    "SIG_IN_FUNC226",
    "SIG_IN_FUNC227",
    "SIG_IN_FUNC228",
};

static const char* out_id[] = {
    "SPICLK_OUT",
    "SPIQ_OUT",
    "SPID_OUT",
    "SPIHD_OUT",
    "SPIWP_OUT",
    "SPICS0_OUT",
    "SPICS1_OUT",
    "SPICS2_OUT",
    "HSPICLK_OUT",
    "HSPIQ_OUT",
    "HSPID_OUT",
    "HSPICS0_OUT",
    "HSPIHD_OUT",
    "HSPIWP_OUT",
    "U0TXD_OUT",
    "U0RTS_OUT",
    "U0DTR_OUT",
    "U1TXD_OUT",
    "U1RTS_OUT",
    "I2CM_SCL_O",
    "I2CM_SDA_O",
    "EXT_I2C_SCL_O",
    "EXT_I2C_SDA_O",
    "I2S0O_BCK_OUT",
    "I2S1O_BCK_OUT",
    "I2S0O_WS_OUT",
    "I2S1O_WS_OUT",
    "I2S0I_BCK_OUT",
    "I2S0I_WS_OUT",
    "I2CEXT0_SCL_OUT",
    "I2CEXT0_SDA_OUT",
    "UNDEF",
    "PWM0_OUT0A",
    "PWM0_OUT0B",
    "PWM0_OUT1A",
    "PWM0_OUT1B",
    "PWM0_OUT2A",
    "PWM0_OUT2B",
    "GPIO_BT_PRIORITY",
    "UNDEF",
    "GPIO_WLAN_ACTIVE",
    "BB_DIAG0",
    "BB_DIAG1",
    "BB_DIAG2",
    "BB_DIAG3",
    "BB_DIAG4",
    "BB_DIAG5",
    "BB_DIAG6",
    "BB_DIAG7",
    "BB_DIAG8",
    "BB_DIAG9",
    "BB_DIAG10",
    "BB_DIAG11",
    "BB_DIAG12",
    "BB_DIAG13",
    "BB_DIAG14",
    "BB_DIAG15",
    "BB_DIAG16",
    "BB_DIAG17",
    "BB_DIAG18",
    "BB_DIAG19",
    "HSPICS1_OUT",
    "HSPICS2_OUT",
    "VSPICLK_OUT",
    "VSPIQ_OUT",
    "VSPID_OUT",
    "VSPIHD_OUT",
    "VSPIWP_OUT",
    "VSPICS0_OUT",
    "VSPICS1_OUT",
    "VSPICS2_OUT",
    "LEDC_HS_SIG_OUT0",
    "LEDC_HS_SIG_OUT1",
    "LEDC_HS_SIG_OUT2",
    "LEDC_HS_SIG_OUT3",
    "LEDC_HS_SIG_OUT4",
    "LEDC_HS_SIG_OUT5",
    "LEDC_HS_SIG_OUT6",
    "LEDC_HS_SIG_OUT7",
    "LEDC_LS_SIG_OUT0",
    "LEDC_LS_SIG_OUT1",
    "LEDC_LS_SIG_OUT2",
    "LEDC_LS_SIG_OUT3",
    "LEDC_LS_SIG_OUT4",
    "LEDC_LS_SIG_OUT5",
    "LEDC_LS_SIG_OUT6",
    "LEDC_LS_SIG_OUT7",
    "RMT_SIG_OUT0",
    "RMT_SIG_OUT1",
    "RMT_SIG_OUT2",
    "RMT_SIG_OUT3",
    "RMT_SIG_OUT4",
    "RMT_SIG_OUT5",
    "RMT_SIG_OUT6",
    "RMT_SIG_OUT7",
    "I2CEXT1_SCL_OUT",
    "I2CEXT1_SDA_OUT",
    "HOST_CCMD_OD_PULLUP_EN_N",
    "HOST_RST_N_1",
    "HOST_RST_N_2",
    "GPIO_SD0_OUT",
    "GPIO_SD1_OUT",
    "GPIO_SD2_OUT",
    "GPIO_SD3_OUT",
    "GPIO_SD4_OUT",
    "GPIO_SD5_OUT",
    "GPIO_SD6_OUT",
    "GPIO_SD7_OUT",
    "PWM1_OUT0A",
    "PWM1_OUT0B",
    "PWM1_OUT1A",
    "PWM1_OUT1B",
    "PWM1_OUT2A",
    "PWM1_OUT2B",
    "PWM2_OUT1H",
    "PWM2_OUT1L",
    "PWM2_OUT2H",
    "PWM2_OUT2L",
    "PWM2_OUT3H",
    "PWM2_OUT3L",
    "PWM2_OUT4H",
    "PWM2_OUT4L",
    "UNDEF",
    "CAN_TX",
    "CAN_BUS_OFF_ON",
    "CAN_CLKOUT",
    "UNDEF",
    "UNDEF",
    "SPID4_OUT",
    "SPID5_OUT",
    "SPID6_OUT",
    "SPID7_OUT",
    "HSPID4_OUT",
    "HSPID5_OUT",
    "HSPID6_OUT",
    "HSPID7_OUT",
    "VSPID4_OUT",
    "VSPID5_OUT",
    "VSPID6_OUT",
    "VSPID7_OUT",
    "I2S0O_DATA_OUT0",
    "I2S0O_DATA_OUT1",
    "I2S0O_DATA_OUT2",
    "I2S0O_DATA_OUT3",
    "I2S0O_DATA_OUT4",
    "I2S0O_DATA_OUT5",
    "I2S0O_DATA_OUT6",
    "I2S0O_DATA_OUT7",
    "I2S0O_DATA_OUT8",
    "I2S0O_DATA_OUT9",
    "I2S0O_DATA_OUT10",
    "I2S0O_DATA_OUT11",
    "I2S0O_DATA_OUT12",
    "I2S0O_DATA_OUT13",
    "I2S0O_DATA_OUT14",
    "I2S0O_DATA_OUT15",
    "I2S0O_DATA_OUT16",
    "I2S0O_DATA_OUT17",
    "I2S0O_DATA_OUT18",
    "I2S0O_DATA_OUT19",
    "I2S0O_DATA_OUT20",
    "I2S0O_DATA_OUT21",
    "I2S0O_DATA_OUT22",
    "I2S0O_DATA_OUT23",
    "I2S1I_BCK_OUT",
    "I2S1I_WS_OUT",
    "I2S1O_DATA_OUT0",
    "I2S1O_DATA_OUT1",
    "I2S1O_DATA_OUT2",
    "I2S1O_DATA_OUT3",
    "I2S1O_DATA_OUT4",
    "I2S1O_DATA_OUT5",
    "I2S1O_DATA_OUT6",
    "I2S1O_DATA_OUT7",
    "I2S1O_DATA_OUT8",
    "I2S1O_DATA_OUT9",
    "I2S1O_DATA_OUT10",
    "I2S1O_DATA_OUT11",
    "I2S1O_DATA_OUT12",
    "I2S1O_DATA_OUT13",
    "I2S1O_DATA_OUT14",
    "I2S1O_DATA_OUT15",
    "I2S1O_DATA_OUT16",
    "I2S1O_DATA_OUT17",
    "I2S1O_DATA_OUT18",
    "I2S1O_DATA_OUT19",
    "I2S1O_DATA_OUT20",
    "I2S1O_DATA_OUT21",
    "I2S1O_DATA_OUT22",
    "I2S1O_DATA_OUT23",
    "PWM3_OUT1H",
    "PWM3_OUT1L",
    "PWM3_OUT2H",
    "PWM3_OUT2L",
    "PWM3_OUT3H",
    "PWM3_OUT3L",
    "PWM3_OUT4H",
    "PWM3_OUT4L",
    "U2TXD_OUT",
    "U2RTS_OUT",
    "EMAC_MDC_O",
    "EMAC_MDO_O",
    "EMAC_CRS_O",
    "EMAC_COL_O",
    "BT_AUDIO0_IRQ",
    "BT_AUDIO1_IRQ",
    "BT_AUDIO2_IRQ",
    "BLE_AUDIO0_IRQ",
    "BLE_AUDIO1_IRQ",
    "BLE_AUDIO2_IRQ",
    "PCMFSYNC_OUT",
    "PCMCLK_OUT",
    "PCMDOUT",
    "BLE_AUDIO_SYNC0_P",
    "BLE_AUDIO_SYNC1_P",
    "BLE_AUDIO_SYNC2_P",
    "ANT_SEL0",
    "ANT_SEL1",
    "ANT_SEL2",
    "ANT_SEL3",
    "ANT_SEL4",
    "ANT_SEL5",
    "ANT_SEL6",
    "ANT_SEL7",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "UNDEF",
    "SIG_GPIO_OUT",
};

// class AttachedGpioInput : public FakeGpioPin {
//  public:
//   AttachedGpioInput(int idx) : FakeGpioPin(in_id[idx]) {}

//   float read() const override {
//     LOG(WARNING) << "Direct read from " << name();
//     return FakeGpioPin::read();
//   }

//   void onWrite(float voltage) override {
//     LOG(WARNING) << "Direct write to " << name();
//     FakeGpioPin::onWrite(voltage);
//   }

//   std::string name() const override {
//     std::set<const char*> signal_names;
//     for (const auto& i : signals_) {
//       signal_names.insert(in_id[i]);
//     }
//     return "InternalInput" + signal_names;
//   }

//  private:
//   std::set<uint8_t> signals_;
// };

// class AttachedGpioOutput : public FakeGpioPin {
//  public:
//   AttachedGpioOutput(int idx) : FakeGpioPin(out_id[idx]) {}

//   float read() const override {
//     LOG(WARNING) << "Direct read from " << *this;
//     return FakeGpioPin::read();
//   }

//   void onWrite(float voltage) override {
//     LOG(WARNING) << "Direct write to " << *this;
//     FakeGpioPin::onWrite(voltage);
//   }
// };

}  // namespace

Esp32InMatrix::Esp32InMatrix() {
  std::fill(&signal_to_pin_[0], &signal_to_pin_[256], kMatrixDetachInUndefPin);
}

Esp32OutMatrix::Esp32OutMatrix() {
  std::fill(&pin_to_signal_[0], &pin_to_signal_[40], kMatrixDetachInUndefPin);
}

Esp32SpiInterface::Esp32SpiInterface(volatile spi_def_t& spi,
                                     const std::string& name,
                                     uint8_t clk_signal, uint8_t miso_signal,
                                     uint8_t mosi_signal, FakeEsp32Board* esp32)
    : FakeSpiInterface(name),
      spi_(spi),
      clk_signal_(clk_signal),
      miso_signal_(miso_signal),
      mosi_signal_(mosi_signal),
      esp32_(esp32) {}

uint32_t Esp32SpiInterface::clkHz() const {
  return spiClockDivToFrequency(spi_.clock.val);
}

SpiDataMode Esp32SpiInterface::dataMode() const {
  switch ((spi_.pin.ck_idle_edge << 1) + spi_.user.ck_out_edge) {
    case 0:
      return kSpiMode0;
    case 1:
      return kSpiMode1;
    case 2:
      return kSpiMode3;
    case 3:
      return kSpiMode2;
  }
}

SpiBitOrder Esp32SpiInterface::bitOrder() const {
  return spi_.ctrl.wr_bit_order ? kSpiMsbFirst : kSpiLsbFirst;
}

uint32_t Esp32SpiInterface::transfer() {
  SimpleFakeSpiDevice* input_dev = nullptr;
  uint8_t buf[64];
  int mosi_bits = spi_.mosi_dlen.usr_mosi_dbitlen + 1;
  int miso_bits = spi_.miso_dlen.usr_miso_dbitlen;
  // Find the device to transfer to.
  for (const auto& i : esp32_->spi_devices()) {
    if (esp32_->out_matrix.signal_for_pin(i.second.clk) != clk_signal_)
      continue;
    SimpleFakeSpiDevice* dev = i.first;
    if (!dev->isSelected()) continue;
    if (esp32_->in_matrix.pin_for_signal(miso_signal_) == i.second.miso) {
      if (input_dev != nullptr) {
        LOG(ERROR) << "SPI interface " << name() << ": bus conflict: selected "
                   << input_dev->name() << " and " << dev->name() << "\n";
        memset((void*)spi_.data_buf, 0xFF, (miso_bits + 7) / 8);
      } else {
        input_dev = dev;
      }
    }
    if (esp32_->out_matrix.signal_for_pin(i.second.mosi) == mosi_signal_) {
      memcpy(buf, (const void*)spi_.data_buf, (mosi_bits + 7) / 8);
    } else {
      FakeGpioPin& pin = esp32_->gpio.get(i.second.mosi);
      if (pin.last_written() >= 1.8) {
        memset(buf, 0xFF, (mosi_bits + 7) / 8);
      } else {
        memset(buf, 0x00, (mosi_bits + 7) / 8);
      }
    }
    dev->transfer(*this, buf, mosi_bits);
    if (dev == input_dev) {
      memcpy((void*)spi_.data_buf, buf, (miso_bits + 7) / 8);
    }
  }
  if (input_dev == nullptr) {
    FakeGpioPin& pin =
        esp32_->gpio.get(esp32_->in_matrix.pin_for_signal(miso_signal_));
    if (pin.digitalRead() == FakeGpioPin::kDigitalHigh) {
      memset((void*)spi_.data_buf, 0xFF, (miso_bits + 7) / 8);
    } else {
      memset((void*)spi_.data_buf, 0x00, (miso_bits + 7) / 8);
    }
  }
  return mosi_bits;
}

void spiFakeTransferOnDevice(int8_t spi_num) {
  auto& spi = FakeEsp32().spi[spi_num];
  uint64_t cycles = spi.transfer();
  auto lag = std::chrono::nanoseconds(1000000000LL * cycles / spi.clkHz());
  FakeEsp32().time().lag(lag);
  FakeEsp32().time().sync();
}

uint32_t Esp32SpiUsr::operator=(uint32_t val) const volatile {
  if (val == 0) return val;

  if (this == &SPI0.cmd.usr) {
    spiFakeTransferOnDevice(0);
  }
  if (this == &SPI1.cmd.usr) {
    spiFakeTransferOnDevice(1);
  }
  if (this == &SPI2.cmd.usr) {
    spiFakeTransferOnDevice(2);
  }
  if (this == &SPI3.cmd.usr) {
    spiFakeTransferOnDevice(3);
  }
  return val;
}

uint32_t Esp32GpioOutW1tc::operator=(uint32_t val) const {
  uint32_t result = val;
  for (int i = 0; i < 32; ++i) {
    if (val & 1) FakeEsp32().gpio.get(i).digitalWriteLow();
    val >>= 1;
  }
  return result;
}

uint32_t Esp32GpioOutW1ts::operator=(uint32_t val) const {
  uint32_t result = val;
  for (int i = 0; i < 32; ++i) {
    if (val & 1) FakeEsp32().gpio.get(i).digitalWriteHigh();
    val >>= 1;
  }
  return result;
}

uint32_t Esp32GpioOut1W1tc::operator=(uint32_t val) const {
  uint32_t result = val;
  for (int i = 32; i < 40; ++i) {
    if (val & 1) FakeEsp32().gpio.get(i).digitalWriteLow();
    val >>= 1;
  }
  return result;
}

uint32_t Esp32GpioOut1W1ts::operator=(uint32_t val) const {
  uint32_t result = val;
  for (int i = 32; i < 40; ++i) {
    if (val & 1) FakeEsp32().gpio.get(i).digitalWriteHigh();
    val >>= 1;
  }
  return result;
}

Esp32GpioInReadSpec::operator uint32_t() const {
  uint32_t val = mask;
  uint32_t result = 0;
  for (int i = 0; i < 32; ++i) {
    if (val & 1) {
      result |= (FakeEsp32().gpio.get(i).read() > 1.7) << i;
    }
    val >>= 1;
  }
  return result;
}

Esp32GpioIn1ReadSpec::operator uint32_t() const {
  uint32_t val = mask;
  uint32_t result = 0;
  for (int i = 32; i < 40; ++i) {
    if (val & 1) {
      result |= (FakeEsp32().gpio.get(i).read() > 1.7) << (i - 32);
    }
    val >>= 1;
  }
  return result;
}

namespace {
std::string default_fs_root_path() {
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) != nullptr) {
    return std::string(cwd) + "/fs_root";
  } else {
    LOG(ERROR) << "getcwd() failed";
    return "";
  }
}
}  // namespace

FakeEsp32Board::FakeEsp32Board()
    : gpio(),
      in_matrix(),
      out_matrix(),
      i2c{FakeI2cInterface("i2c0"), FakeI2cInterface("i2c1")},
      spi{Esp32SpiInterface(SPI0, "spi0(internal)", SPICLK_OUT_IDX,
                            SPIQ_OUT_IDX, SPID_IN_IDX, this),
          Esp32SpiInterface(SPI1, "spi1(FSPI)", SPICLK_OUT_IDX, SPIQ_OUT_IDX,
                            SPID_IN_IDX, this),
          Esp32SpiInterface(SPI2, "spi2(HSPI)", HSPICLK_OUT_IDX, HSPIQ_OUT_IDX,
                            HSPID_IN_IDX, this),
          Esp32SpiInterface(SPI3, "spi3(VSPI)", VSPICLK_OUT_IDX, VSPIQ_OUT_IDX,
                            VSPID_IN_IDX, this)},
      fs_root_(default_fs_root_path()),
      time_([this]() { flush(); }) {}

void FakeEsp32Board::attachSpiDevice(SimpleFakeSpiDevice& dev, int8_t clk,
                                     int8_t miso, int8_t mosi) {
  spi_devices_to_pins_[&dev] = SpiPins{
      .clk = clk,
      .miso = miso,
      .mosi = mosi,
  };
}

void FakeEsp32Board::flush() {
  for (auto& dev : spi_devices_to_pins_) {
    dev.first->flush();
  }
}

void Esp32InMatrix::assign(uint32_t gpio, uint32_t signal_idx, bool inv) {
  uint8_t old_pin = signal_to_pin_[signal_idx];
  signal_to_pin_[signal_idx] = gpio;
  pin_to_signals_.erase(old_pin);
  if (gpio != kMatrixDetachInUndefPin) {
    pin_to_signals_[gpio] = signal_idx;
  }
}

void Esp32OutMatrix::assign(uint32_t gpio, uint32_t signal_idx, bool out_inv,
                            bool oen_inv) {
  uint16_t old_signal = pin_to_signal_[gpio];
  signal_to_pins_.erase(old_signal);
  pin_to_signal_[gpio] = signal_idx;
  if (gpio == kMatrixDetachOutSig) {
    signal_to_pins_.erase(signal_idx);
  } else {
    signal_to_pins_[signal_idx] = gpio;
  }
}

void EmulatedTime::sync() const {
  auto rt = rt_clock_.now() - rt_start_time_;
  if (rt > emu_uptime_ && rt - emu_uptime_ > kMaxTimeLag) {
    emu_uptime_ = rt;
  } else if (rt < emu_uptime_ && emu_uptime_ - rt > kMaxTimeAhead) {
    std::this_thread::sleep_for(emu_uptime_ - rt - kMaxTimeAhead);
    // Adjust the emulated time in case we overslept.
    rt = rt_clock_.now() - rt_start_time_;
    if (rt > emu_uptime_) {
      emu_uptime_ = rt;
    }
  }
}

void EmulatedTime::lag(std::chrono::nanoseconds lag) { emu_uptime_ += lag; }

int64_t EmulatedTime::getTimeMicros() const {
  sync();
  return std::chrono::duration_cast<std::chrono::microseconds>(emu_uptime_)
      .count();
}

void EmulatedTime::delayMicros(uint64_t delay) {
  lag(std::chrono::microseconds(delay));
  sync();
}

namespace roo_testing_transducers {

Clock* getDefaultSystemClock() {
  return &FakeEsp32().time();
}

}  // namespace