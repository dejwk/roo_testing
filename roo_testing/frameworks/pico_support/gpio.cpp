#include "hardware/gpio.h"

#include <cstdlib>

#include "glog/logging.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

namespace {

/// Forms a checked single-pin mask without an undefined-width shift.
uint32_t PinMask(uint pin) {
  FakePicoBoard::CheckPin(pin);
  return uint32_t{1} << pin;
}

/// Rejects high-bank and reserved mask bits before narrowing SDK 64-bit masks.
uint32_t Mask32(uint64_t mask) {
  FakePicoBoard::CheckMask(mask);
  return static_cast<uint32_t>(mask);
}

/// Rejects operations on nonexistent RP2040 GPIO banks.
void CheckBank(uint bank) { CHECK_EQ(bank, 0u) << "Invalid Pico GPIO bank"; }

/// Reports a requested peripheral feature without a successful placeholder.
[[noreturn]] void Unsupported(const char* operation) {
  LOG(FATAL) << "Unimplemented: Pico GPIO " << operation;
  std::abort();
}

}  // namespace

extern "C" {

void check_gpio_param(uint gpio) { FakePicoBoard::CheckPin(gpio); }

void gpio_init(uint gpio) { FakePico().init(PinMask(gpio)); }

void gpio_deinit(uint gpio) {
  FakePico().setFunction(PinMask(gpio), GPIO_FUNC_NULL);
}

void gpio_init_mask(uint32_t mask) { FakePico().init(mask); }

void gpio_init_mask64(uint64_t mask) { FakePico().init(Mask32(mask)); }

void gpio_set_function(uint gpio, gpio_function_t fn) {
  FakePico().setFunction(PinMask(gpio), fn);
}

void gpio_set_function_masked(uint32_t mask, gpio_function_t fn) {
  FakePico().setFunction(mask, fn);
}

void gpio_set_function_masked64(uint64_t mask, gpio_function_t fn) {
  FakePico().setFunction(Mask32(mask), fn);
}

gpio_function_t gpio_get_function(uint gpio) {
  return FakePico().state(gpio).function;
}

void gpio_set_pulls(uint gpio, bool up, bool down) {
  FakePico().setPulls(gpio, up, down);
}

void gpio_pull_up(uint gpio) { gpio_set_pulls(gpio, true, false); }

void gpio_pull_down(uint gpio) { gpio_set_pulls(gpio, false, true); }

void gpio_disable_pulls(uint gpio) { gpio_set_pulls(gpio, false, false); }

bool gpio_is_pulled_up(uint gpio) { return FakePico().state(gpio).pull_up; }

bool gpio_is_pulled_down(uint gpio) { return FakePico().state(gpio).pull_down; }

void gpio_set_input_enabled(uint gpio, bool enabled) {
  FakePico().setInputEnabled(gpio, enabled);
}

void gpio_set_inover(uint gpio, uint value) {
  FakePico().setInputOverride(gpio, value);
}

void gpio_set_outover(uint gpio, uint value) {
  FakePico().setOutputOverride(gpio, value);
}

void gpio_set_oeover(uint gpio, uint value) {
  FakePico().setOutputEnableOverride(gpio, value);
}

bool gpio_get(uint gpio) { return FakePico().read(gpio); }

uint32_t gpio_get_all(void) { return FakePico().readAll(); }

uint64_t gpio_get_all64(void) { return gpio_get_all(); }

void gpio_put(uint gpio, int value) {
  FakePico().put(PinMask(gpio), value != 0 ? PinMask(gpio) : 0);
}

bool gpio_get_out_level(uint gpio) {
  return FakePico().state(gpio).output_latch;
}

void gpio_put_masked(uint32_t mask, uint32_t value) {
  FakePico().put(mask, value);
}

void gpio_put_masked64(uint64_t mask, uint64_t value) {
  gpio_put_masked(Mask32(mask), static_cast<uint32_t>(value));
}

void gpio_put_masked_n(uint n, uint32_t mask, uint32_t value) {
  CheckBank(n);
  gpio_put_masked(mask, value);
}

void gpio_put_all(uint32_t value) {
  FakePicoBoard::CheckMask(value);
  gpio_put_masked(FakePicoBoard::kGpioMask, value);
}

void gpio_put_all64(uint64_t value) { gpio_put_all(Mask32(value)); }

void gpio_set_dir(uint gpio, bool out) {
  FakePico().setDirection(PinMask(gpio), out ? PinMask(gpio) : 0);
}

void gpio_set_dir_masked(uint32_t mask, uint32_t value) {
  FakePico().setDirection(mask, value);
}

void gpio_set_dir_masked64(uint64_t mask, uint64_t value) {
  gpio_set_dir_masked(Mask32(mask), static_cast<uint32_t>(value));
}

void gpio_set_dir_all_bits(uint32_t value) {
  FakePicoBoard::CheckMask(value);
  gpio_set_dir_masked(FakePicoBoard::kGpioMask, value);
}

void gpio_set_dir_all_bits64(uint64_t value) {
  gpio_set_dir_all_bits(Mask32(value));
}

bool gpio_is_dir_out(uint gpio) { return FakePico().state(gpio).direction_out; }

uint gpio_get_dir(uint gpio) {
  return gpio_is_dir_out(gpio) ? GPIO_OUT : GPIO_IN;
}

void gpio_set_mask(uint32_t mask) { FakePico().put(mask, mask); }

void gpio_set_mask64(uint64_t mask) { gpio_set_mask(Mask32(mask)); }

void gpio_set_mask_n(uint n, uint32_t mask) {
  CheckBank(n);
  gpio_set_mask(mask);
}

void gpio_clr_mask(uint32_t mask) { FakePico().put(mask, 0); }

void gpio_clr_mask64(uint64_t mask) { gpio_clr_mask(Mask32(mask)); }

void gpio_clr_mask_n(uint n, uint32_t mask) {
  CheckBank(n);
  gpio_clr_mask(mask);
}

void gpio_xor_mask(uint32_t mask) { FakePico().toggle(mask); }

void gpio_xor_mask64(uint64_t mask) { gpio_xor_mask(Mask32(mask)); }

void gpio_xor_mask_n(uint n, uint32_t mask) {
  CheckBank(n);
  gpio_xor_mask(mask);
}

void gpio_set_dir_out_masked(uint32_t mask) {
  FakePico().setDirection(mask, mask);
}

void gpio_set_dir_out_masked64(uint64_t mask) {
  gpio_set_dir_out_masked(Mask32(mask));
}

void gpio_set_dir_in_masked(uint32_t mask) { FakePico().setDirection(mask, 0); }

void gpio_set_dir_in_masked64(uint64_t mask) {
  gpio_set_dir_in_masked(Mask32(mask));
}

void gpio_set_irqover(uint gpio, uint) {
  FakePicoBoard::CheckPin(gpio);
  Unsupported("interrupts");
}

void gpio_set_irq_enabled(uint gpio, uint32_t, bool) {
  FakePicoBoard::CheckPin(gpio);
  Unsupported("interrupts");
}

void gpio_set_irq_callback(gpio_irq_callback_t) { Unsupported("interrupts"); }

void gpio_set_irq_enabled_with_callback(uint gpio, uint32_t, bool,
                                        gpio_irq_callback_t) {
  FakePicoBoard::CheckPin(gpio);
  Unsupported("interrupts");
}

void gpio_set_dormant_irq_enabled(uint gpio, uint32_t, bool) {
  FakePicoBoard::CheckPin(gpio);
  Unsupported("interrupts");
}

uint32_t gpio_get_irq_event_mask(uint gpio) {
  FakePicoBoard::CheckPin(gpio);
  Unsupported("interrupts");
}

void gpio_acknowledge_irq(uint gpio, uint32_t) {
  FakePicoBoard::CheckPin(gpio);
  Unsupported("interrupts");
}

void gpio_add_raw_irq_handler_with_order_priority_masked(uint32_t mask,
                                                         irq_handler_t,
                                                         uint8_t) {
  FakePicoBoard::CheckMask(mask);
  Unsupported("interrupts");
}

void gpio_add_raw_irq_handler_with_order_priority_masked64(uint64_t mask,
                                                           irq_handler_t,
                                                           uint8_t) {
  FakePicoBoard::CheckMask(mask);
  Unsupported("interrupts");
}

void gpio_add_raw_irq_handler_with_order_priority(uint gpio, irq_handler_t,
                                                  uint8_t) {
  FakePicoBoard::CheckPin(gpio);
  Unsupported("interrupts");
}

void gpio_add_raw_irq_handler_masked(uint32_t mask, irq_handler_t) {
  FakePicoBoard::CheckMask(mask);
  Unsupported("interrupts");
}

void gpio_add_raw_irq_handler_masked64(uint64_t mask, irq_handler_t) {
  FakePicoBoard::CheckMask(mask);
  Unsupported("interrupts");
}

void gpio_add_raw_irq_handler(uint gpio, irq_handler_t) {
  FakePicoBoard::CheckPin(gpio);
  Unsupported("interrupts");
}

void gpio_remove_raw_irq_handler_masked(uint32_t mask, irq_handler_t) {
  FakePicoBoard::CheckMask(mask);
  Unsupported("interrupts");
}

void gpio_remove_raw_irq_handler_masked64(uint64_t mask, irq_handler_t) {
  FakePicoBoard::CheckMask(mask);
  Unsupported("interrupts");
}

void gpio_remove_raw_irq_handler(uint gpio, irq_handler_t) {
  FakePicoBoard::CheckPin(gpio);
  Unsupported("interrupts");
}

void gpio_set_input_hysteresis_enabled(uint gpio, bool) {
  FakePicoBoard::CheckPin(gpio);
  Unsupported("analog pad controls");
}

bool gpio_is_input_hysteresis_enabled(uint gpio) {
  FakePicoBoard::CheckPin(gpio);
  Unsupported("analog pad controls");
}

void gpio_set_slew_rate(uint gpio, enum gpio_slew_rate) {
  FakePicoBoard::CheckPin(gpio);
  Unsupported("analog pad controls");
}

enum gpio_slew_rate gpio_get_slew_rate(uint gpio) {
  FakePicoBoard::CheckPin(gpio);
  Unsupported("analog pad controls");
}

void gpio_set_drive_strength(uint gpio, enum gpio_drive_strength) {
  FakePicoBoard::CheckPin(gpio);
  Unsupported("analog pad controls");
}

enum gpio_drive_strength gpio_get_drive_strength(uint gpio) {
  FakePicoBoard::CheckPin(gpio);
  Unsupported("analog pad controls");
}

void gpio_debug_pins_init(void) { Unsupported("debug pin setup"); }

}  // extern "C"
