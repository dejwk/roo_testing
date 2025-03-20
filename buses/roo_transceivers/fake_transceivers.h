#pragma once

#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "roo_transceivers/id.h"
#include "roo_transceivers/universe.h"

class FakeTransceiver {
 public:
  FakeTransceiver(roo_transceivers::DeviceLocator locator,
                  const roo_transceivers_Descriptor* descriptor)
      : locator_(locator), descriptor_(descriptor) {}

  virtual ~FakeTransceiver() = default;

  const roo_transceivers::DeviceLocator& locator() const { return locator_; }
  const roo_transceivers_Descriptor& descriptor() const { return *descriptor_; }

  virtual roo_transceivers::Measurement read(
      std::string_view sensor_id) const = 0;
  virtual bool write(std::string_view actuator_id, float value) = 0;

  virtual void requestUpdate() {}

 private:
  roo_transceivers::DeviceLocator locator_;
  const roo_transceivers_Descriptor* descriptor_;
};

class FakeTransceiverUniverse : public roo_transceivers::Universe {
 public:
  FakeTransceiverUniverse(const std::vector<FakeTransceiver*> transceivers) {
    for (auto t : transceivers) {
      transceivers_[t->locator()] = t;
    }
  }

  size_t deviceCount() const override { return transceivers_.size(); }

  bool forEachDevice(std::function<bool(const roo_transceivers::DeviceLocator&)>
                         callback) const override {
    for (auto itr : transceivers_) {
      if (!callback(itr.first)) return false;
    }
    return true;
  }

  bool getDeviceDescriptor(
      const roo_transceivers::DeviceLocator& locator,
      roo_transceivers_Descriptor& descriptor) const override {
    const FakeTransceiver* transceiver = findTransceiver(locator);
    if (transceiver == nullptr) return false;
    descriptor = transceiver->descriptor();
    return true;
  }

  roo_transceivers::Measurement read(
      const roo_transceivers::SensorLocator& locator) const override {
    FakeTransceiver* transceiver = findTransceiver(locator.device_locator());
    if (transceiver == nullptr) return roo_transceivers::Measurement();
    return transceiver->read(locator.sensor_id());
  }

  bool write(const roo_transceivers::ActuatorLocator& locator,
             float value) const override {
    FakeTransceiver* transceiver = findTransceiver(locator.device_locator());
    if (transceiver == nullptr) return false;
    return transceiver->write(locator.actuator_id(), value);
  }

  virtual void requestUpdate() {
    for (auto itr : transceivers_) {
      itr.second->requestUpdate();
    }
  }

  void addEventListener(roo_transceivers::EventListener* listener) override {
    listeners_.insert(listener);
  }

  void removeEventListener(roo_transceivers::EventListener* listener) {
    listeners_.erase(listener);
  }

 private:
  FakeTransceiver* findTransceiver(
      const roo_transceivers::DeviceLocator& locator) const {
    auto itr = transceivers_.find(locator);
    return (itr == transceivers_.end()) ? nullptr : itr->second;
  }

  //   FakeTransceiver* findTransceiver(
  //       const roo_transceivers::DeviceLocator& locator) {
  //     auto itr = transceivers_.find(locator);
  //     return (itr == transceivers_.end()) ? nullptr : itr->second;
  //   }

  std::unordered_map<roo_transceivers::DeviceLocator, FakeTransceiver*>
      transceivers_;
  std::unordered_set<roo_transceivers::EventListener*> listeners_;
};
