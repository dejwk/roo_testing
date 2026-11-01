#include <Arduino.h>

#include "roo_display/hal/arduino/spi.h"
#include "roo_display/hal/spi_settings.h"
#include "roo_scheduler.h"
#include "roo_time.h"

#if defined(ROO_TESTING) || !PICO_ON_DEVICE || !PICO_RP2040
#error Roo consumers must compile against real RP2040 device headers
#endif

// Verifies the selected Roo subset parses against the real Arduino-Pico API.
void CompileRooPicoConsumers() {
  roo_scheduler::SchedulingService scheduler;
  scheduler.scheduleAfter(roo_time::Millis(1), [] {});
  scheduler.delay(roo_time::Millis(2));
  roo_display::ArduinoSpi bus;
  bus.init(18, 16, 19);
  using Settings = roo_display::SpiSettings<1000000, roo_display::kSpiMsbFirst,
                                            roo_display::kSpiMode0>;
  roo_display::ArduinoSpiDevice<Settings> display(bus);
  display.beginWriteOnlyTransaction();
  display.write16(0xf800);
  display.endTransaction();
}
