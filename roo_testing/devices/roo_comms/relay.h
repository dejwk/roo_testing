// #pragma once

// #include <string_view>
// #include <unordered_map>
// #include <unordered_set>
// #include <vector>

// #include "roo_testing/buses/roo_transceivers/fake_transceivers.h"
// #include "roo_testing/transducers/voltage/voltage.h"
// #include "roo_transceivers/id.h"
// #include "roo_transceivers/universe.h"

// roo_transceivers_Descriptor* getFakeRooRelay4pDescriptor();

// class FakeRooRelay4p : public FakeTransceiver {
//  public:
//   using WriteFn = std::function<void(int, const FakeRooRelay4p& relay,
//                                      roo_testing_transducers::DigitalLevel)>;

//   FakeRooRelay4p(const roo_transceivers::DeviceLocator& locator,
//                  WriteFn write_fn)
//       : FakeTransceiver(locator, getFakeRooRelay4pDescriptor()),
//         write_fn_(std::move(write_fn)) {
//     levels_[0] = roo_testing_transducers::kDigitalHigh;
//     levels_[1] = roo_testing_transducers::kDigitalHigh;
//     levels_[2] = roo_testing_transducers::kDigitalHigh;
//     levels_[3] = roo_testing_transducers::kDigitalHigh;
//   }

//   roo_testing_transducers::DigitalLevel getLevel(int idx) const {
//     return levels_[idx];
//   }

//   roo_transceivers::Measurement read(std::string_view sensor_id) const override;

//   virtual bool write(std::string_view actuator_id, float value) override;

//  private:
//   roo_testing_transducers::DigitalLevel levels_[4];
//   WriteFn write_fn_;
// };
