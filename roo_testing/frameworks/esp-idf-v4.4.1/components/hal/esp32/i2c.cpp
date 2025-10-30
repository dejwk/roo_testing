#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "soc/i2c_struct.h"

extern "C" {

void i2c_ll_txfifo_rst(i2c_dev_t* hw) {
  int idx = (hw == &I2C1);
  FakeEsp32().i2c(idx).resetTxFifo();
}

void i2c_ll_trans_start(i2c_dev_t* hw) {
  hw->ctr.trans_start = 1;

  int dev_idx = (hw == &I2C1);
  auto& dev = FakeEsp32().i2c(dev_idx);

  int cmd_idx = 0;
  while (true) {
    const auto& cmd = hw->command[cmd_idx];
    switch (cmd.op_code) {
      case 0: {
        dev.start();
        break;
      }
      case 1: {
        dev.write();
        break;
      }
      case 2: {
        dev.read();
        break;
      }
      case 3: {
        dev.stop();
        return;
      }
      case 4: {
        dev.end();
        return;
      }
    }
    ++cmd_idx;
  }
}
}