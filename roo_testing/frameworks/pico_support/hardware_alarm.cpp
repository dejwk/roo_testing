#include "roo_testing/frameworks/pico_support/hardware_alarm.h"

#include <algorithm>
#include <array>

#include "glog/logging.h"
#include "hardware/sync.h"
#include "pico/time.h"
#include "roo_testing/interrupts/interrupt_controller.h"
#include "roo_testing/interrupts/interrupt_delivery.h"
#include "roo_testing/interrupts/pico_interrupt_delivery.h"

namespace {

using roo_testing::pico::kNoHardwareAlarm;
namespace interrupt_delivery = roo_testing::interrupt_delivery;

struct HardwareAlarm {
  roo_testing::InterruptHandle interrupt;
  hardware_alarm_callback_t callback = nullptr;
  uint64_t deadline = kNoHardwareAlarm;
  bool claimed = false;
  bool fired = false;
};

std::array<HardwareAlarm, NUM_ALARMS> alarms;

/// Finds the checked hardware state for an RP2040 timer alarm.
HardwareAlarm& Alarm(uint number) {
  CHECK_LT(number, alarms.size());
  return alarms[number];
}

/// Calls the timer callback with the Pico core's exception number installed.
void DispatchTimerAlarm(void* argument) {
  HardwareAlarm& alarm = *static_cast<HardwareAlarm*>(argument);
  if (!alarm.fired || alarm.callback == nullptr) return;
  alarm.fired = false;
  const uint number = static_cast<uint>(&alarm - alarms.data());
  interrupt_delivery::EnterPicoTimerInterrupt(number);
  alarm.callback(number);
  interrupt_delivery::LeavePicoTimerInterrupt();
}

/// Asserts a compare or forced interrupt after updating the timer state.
void RaiseTimerAlarm(HardwareAlarm& alarm) {
  alarm.fired = true;
  if (alarm.interrupt.isValid()) {
    CHECK(roo_testing::setInterruptPending(alarm.interrupt));
  }
}

}  // namespace

namespace roo_testing::pico {

void PollTimerInterrupts() {
  const uint64_t now = time_us_64();
  for (HardwareAlarm& alarm : alarms) {
    if (alarm.deadline <= now) {
      alarm.deadline = kNoHardwareAlarm;
      RaiseTimerAlarm(alarm);
    }
  }
  interrupt_delivery::DispatchPicoInterrupts();
}

uint64_t NextHardwareAlarm() {
  uint64_t nearest = kNoHardwareAlarm;
  for (const HardwareAlarm& alarm : alarms) {
    nearest = std::min(nearest, alarm.deadline);
  }
  return nearest;
}

}  // namespace roo_testing::pico

extern "C" {

void hardware_alarm_claim(uint number) {
  HardwareAlarm& alarm = Alarm(number);
  CHECK(!alarm.claimed);
  alarm.claimed = true;
}

void hardware_alarm_unclaim(uint number) {
  HardwareAlarm& alarm = Alarm(number);
  CHECK(alarm.claimed);
  hardware_alarm_cancel(number);
  hardware_alarm_set_callback(number, nullptr);
  alarm.claimed = false;
}

int hardware_alarm_claim_unused(bool required) {
  for (uint number = 0; number < alarms.size(); ++number) {
    if (!alarms[number].claimed) {
      hardware_alarm_claim(number);
      return static_cast<int>(number);
    }
  }
  CHECK(!required) << "No unused Pico hardware alarms";
  return -1;
}

void hardware_alarm_set_callback(uint number, hardware_alarm_callback_t cb) {
  HardwareAlarm& alarm = Alarm(number);
  alarm.callback = cb;
  if (cb == nullptr) {
    alarm.fired = false;
    if (alarm.interrupt.isValid()) {
      CHECK(roo_testing::disableInterrupt(alarm.interrupt));
    }
  } else {
    if (alarm.interrupt.isValid()) {
      CHECK(roo_testing::enableInterrupt(alarm.interrupt));
    } else {
      CHECK(roo_testing::registerInterrupt(DispatchTimerAlarm, &alarm, true,
                                           &alarm.interrupt) ==
            roo_testing::InterruptRegistrationResult::kRegistered);
    }
    if (alarm.fired) RaiseTimerAlarm(alarm);
  }
  roo_testing::pico::PollTimerInterrupts();
}

bool hardware_alarm_set_target(uint number, absolute_time_t target) {
  HardwareAlarm& alarm = Alarm(number);
  const uint64_t deadline = to_us_since_boot(target);
  if (deadline <= time_us_64()) return true;
  CHECK_LE(deadline, static_cast<uint64_t>(INT64_MAX / 1000));
  alarm.deadline = deadline;
  alarm.fired = false;
  return false;
}

void hardware_alarm_cancel(uint number) {
  HardwareAlarm& alarm = Alarm(number);
  alarm.deadline = kNoHardwareAlarm;
  alarm.fired = false;
}

void hardware_alarm_force_irq(uint number) {
  RaiseTimerAlarm(Alarm(number));
  roo_testing::pico::PollTimerInterrupts();
}

uint32_t save_and_disable_interrupts() { return interrupt_delivery::Mask(); }

void disable_interrupts() { interrupt_delivery::Mask(); }

void restore_interrupts(uint32_t status) {
  interrupt_delivery::Restore(status);
  roo_testing::pico::PollTimerInterrupts();
}

void restore_interrupts_from_disabled(uint32_t status) {
  restore_interrupts(status);
}

void enable_interrupts() { restore_interrupts(0); }

void tight_loop_contents() { roo_testing::pico::PollTimerInterrupts(); }

}  // extern "C"
