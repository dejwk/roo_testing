#include "fake_spi.h"

#include <glog/logging.h>

bool SimpleFakeSpiDevice::isSelected() const {
  cs_.warnIfUndef();
  return cs_.isLow();
}

SimpleFakeSpiDevice::SimpleFakeSpiDevice(const std::string& name)
    : name_(name),
      cs_(roo_testing_transducers::SimpleDigitalSink::WithSignalCallback(
          name + ":CS", [this](const roo_testing_transducers::VoltageSignal&) {
            updateChipSelect();
          })) {}

void SimpleFakeSpiDevice::updateChipSelect() {
  const bool selected = cs_.isLow();
  if (selected == selected_) return;
  selected_ = selected;
  onChipSelect(selected);
}
