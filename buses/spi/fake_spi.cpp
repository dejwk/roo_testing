#include "fake_spi.h"

#include <glog/logging.h>

bool SimpleFakeSpiDevice::isSelected() const {
  cs_.warnIfUndef();
  return cs_.isLow();
}
