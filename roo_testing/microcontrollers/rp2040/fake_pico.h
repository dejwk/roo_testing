#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "roo_testing/buses/gpio/fake_gpio.h"
#include "roo_testing/buses/i2c/fake_i2c.h"
#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/buses/uart/fake_uart.h"

/// Owns RP2040 GPIO/UART/SPI/I2C state and borrowed device connections.
/// SDK operations are serialized under the FreeRTOS profile. Fixture wiring is
/// configured before scheduling. Attach devices
/// before configuring their pins and retain them through all GPIO activity.
class FakePicoBoard {
 public:
  /// Keeps the process-wide board topology unique.
  FakePicoBoard(const FakePicoBoard&) = delete;

  /// Prevents replacing the process-wide board state.
  FakePicoBoard& operator=(const FakePicoBoard&) = delete;

  static constexpr uint kGpioCount = 30;
  static constexpr uint32_t kGpioMask = (uint32_t{1} << kGpioCount) - 1;

  /// Captures a pin's software latch, mux, and supported pad controls.
  struct PinState {
    gpio_function_t function = GPIO_FUNC_NULL;
    bool direction_out = false;
    bool output_latch = false;
    bool pull_up = false;
    bool pull_down = true;
    bool input_enabled = true;
    uint input_override = GPIO_OVERRIDE_NORMAL;
    uint output_override = GPIO_OVERRIDE_NORMAL;
    uint output_enable_override = GPIO_OVERRIDE_NORMAL;
  };

  /// Connects a borrowed external source; active output drive is a conflict.
  void attachInput(uint pin,
                   const roo_testing_transducers::VoltageSource& source);

  /// Connects a borrowed observer of strong output voltage assignments.
  void attachOutput(uint pin, roo_testing_transducers::VoltageSink& sink);

  /// Connects borrowed bidirectional I/O; NaN input denotes external high
  /// impedance.
  void attach(uint pin, roo_testing_transducers::VoltageIO& io);

  /// Captures the supported synchronous UART configuration.
  struct UartState {
    bool enabled = false;
    uint baudrate = 0;
    bool translate_crlf = false;
  };

  /// Connects a borrowed byte device through legal TX/RX GPIOs before mux
  /// setup. Use -1 for an unused direction; at least one must be connected.
  /// Both pins must belong to the same UART. Wiring is immutable and reserves
  /// the pins.
  void attachUartDevice(FakeUartDevice& device, int tx, int rx);

  /// Returns a checked UART configuration snapshot for controller 0 or 1.
  UartState uartState(uint index) const;

  /// Initializes 8N1 operation using the nominal 125 MHz peripheral clock.
  /// Returns the divisor-rounded baudrate; zero requests fail before mutation.
  uint initUart(uint index, uint baudrate);

  /// Disables a UART without consuming external device queues or changing
  /// muxes.
  void deinitUart(uint index);

  /// Changes an enabled UART's divisor-rounded baudrate without byte pacing.
  uint setUartBaudrate(uint index, uint baudrate);

  /// Controls LF-to-CRLF conversion for text helpers on an enabled UART.
  void setUartTranslateCrLf(uint index, bool enabled);

  /// Resolves exactly one active TX or RX byte route for an enabled UART.
  /// UART0 GP0/GP1 use the host console unless explicitly attached. Missing,
  /// ambiguous, disabled, or unsupported pad routes fail before device access.
  FakeUartDevice& uartDevice(uint index, bool transmit) const;

  /// Captures the supported synchronous SPI master configuration.
  struct SpiState {
    bool enabled = false;
    bool busy = false;
    uint baudrate = 0;
    uint data_bits = 8;
    SpiDataMode mode = kSpiMode0;
  };

  /// Captures selected shared devices and the single MISO responder for a call.
  struct SpiRoute {
    std::vector<SimpleFakeSpiDevice*> devices;
    SimpleFakeSpiDevice* receiver = nullptr;
    bool idle_high = false;
  };

  /// Connects a borrowed SPI device before pin configuration. SCK/MOSI are
  /// required; MISO may be -1 for a write-only device. Bus pins can be shared
  /// by devices on the same controller. Attach device.cs() separately to SIO
  /// GPIO.
  void attachSpiDevice(SimpleFakeSpiDevice& device, int clk, int miso,
                       int mosi);

  /// Returns a checked controller configuration snapshot.
  SpiState spiState(uint index) const;

  /// Initializes a mode-0, 8-bit master at the achieved nominal-clock baudrate.
  uint initSpi(uint index, uint baudrate);

  /// Disables an idle SPI controller, retaining pins and borrowed connections.
  void deinitSpi(uint index);

  /// Changes an enabled, idle controller's achieved nominal-clock baudrate.
  uint setSpiBaudrate(uint index, uint baudrate);

  /// Changes an idle controller's supported frame width and mode.
  void setSpiFormat(uint index, uint data_bits, spi_cpol_t cpol,
                    spi_cpha_t cpha, spi_order_t order);

  /// Preflights current routes and CS selection without changing device state.
  /// Returns PICO_OK or a negative state/conflict error. Receive with no
  /// selected responder samples the MISO pull; floating or overridden pads
  /// fail.
  int prepareSpi(uint index, bool receive, SpiRoute& route) const;

  /// Marks an idle controller busy until endSpi(), rejecting nested transfers.
  void beginSpi(uint index);

  /// Ends a synchronous controller operation.
  void endSpi(uint index);

  /// Captures seven-bit master configuration and retained START/STOP intent.
  struct I2cState {
    bool enabled = false;
    bool busy = false;
    bool restart_on_next = false;
    uint baudrate = 0;
  };

  /// Borrows an addressed device on legal SDA/SCL pins before configuration.
  /// Shared bus pins are allowed; duplicate devices/addresses on one tuple
  /// fail.
  void attachI2cDevice(FakeI2cDevice& device, int sda, int scl);

  /// Returns a checked I2C controller snapshot.
  I2cState i2cState(uint index) const;

  /// Initializes a master and returns the nominal-clock achieved baudrate.
  uint initI2c(uint index, uint baudrate);

  /// Disables an idle master, clearing retained repeated-START intent.
  void deinitI2c(uint index);

  /// Updates an enabled idle master's nominal-clock divider.
  uint setI2cBaudrate(uint index, uint baudrate);

  /// Resolves current muxes, checked pads, and exactly one addressed device.
  /// Returns a PICO_ERROR before device delivery when routing is invalid.
  int prepareI2c(uint index, uint8_t address, FakeI2cDevice*& device) const;

  /// Marks synchronous device delivery busy, rejecting callback
  /// reconfiguration.
  void beginI2c(uint index);

  /// Ends delivery, retaining repeated-START intent only after successful
  /// nostop.
  void endI2c(uint index, bool restart);

  /// Stores one free-running PWM slice shared by its A/B GPIO aliases.
  struct PwmState {
    bool enabled = false;
    uint16_t divider16 = 16;
    uint16_t wrap = 65535;
    std::array<uint16_t, 2> levels{};
    std::array<bool, 2> inverted{};
    uint16_t counter = 0;
    int64_t origin_us = 0;
  };

  /// Initializes external ADC selection without changing pin configuration.
  void initAdc();

  /// Configures a checked external analog pin after voltage fixture attachment.
  void initAdcGpio(uint pin);

  /// Selects an external channel, rejecting the internal sensor.
  void selectAdc(uint input);

  /// Returns the initialized converter's selected external channel.
  uint adcInput() const;

  /// Reads/clamps the selected pin against the fixed 3.3 V board reference.
  uint16_t readAdc() const;

  /// Returns a checked PWM slice snapshot.
  PwmState pwmState(uint slice) const;

  /// Publishes validated slice state to every currently mapped GPIO alias.
  /// Observers see the complete state; callbacks cannot reconfigure the board.
  void setPwmState(uint slice, const PwmState& state);

  /// Samples the current counter with microsecond uptime resolution.
  uint16_t pwmCounter(uint slice) const;

  /// Returns a checked, read-only snapshot of one pin's configuration.
  PinState state(uint pin) const;

  /// Returns the active strong output signal, or no value for high impedance.
  /// UART/SPI muxes/reservations have no bit waveform and fail explicitly.
  std::optional<roo_testing_transducers::VoltageSignal> driveSignal(
      uint pin) const;

  /// Samples the resolved pad voltage; NaN denotes an undriven floating pad.
  /// External sources override weak pulls. Opposing strong drivers fail.
  /// UART/SPI muxes/reservations have no voltage model and fail explicitly.
  float padVoltage(uint pin) const;

  /// Reads the digital input after input enable and input override.
  /// Unforced floating or intermediate voltages fail instead of guessing a bit.
  bool read(uint pin) const;

  /// Reads all digital inputs into the low 30 bits.
  uint32_t readAll() const;

  /// Initializes selected pins as SIO inputs with a low output latch.
  /// Pull configuration is preserved, matching gpio_init().
  void init(uint32_t mask);

  /// Selects supported SIO, NULL, UART, SPI, or I2C muxes and resets overrides.
  void setFunction(uint32_t mask, gpio_function_t function);

  /// Updates selected raw SIO output latches without requiring output
  /// direction.
  void put(uint32_t mask, uint32_t value);

  /// Toggles selected raw SIO output latches.
  void toggle(uint32_t mask);

  /// Updates selected SIO directions, independent of the output-enable
  /// override.
  void setDirection(uint32_t mask, uint32_t value);

  /// Sets weak pulls; the unsupported two-pull bus keeper fails explicitly.
  void setPulls(uint pin, bool up, bool down);

  /// Enables or disables the pad input before applying the input override.
  void setInputEnabled(uint pin, bool enabled);

  /// Changes the input override to a checked SDK override value.
  void setInputOverride(uint pin, uint value);

  /// Changes the SIO output-level override to a checked SDK override value.
  void setOutputOverride(uint pin, uint value);

  /// Changes the SIO output-enable override to a checked SDK override value.
  void setOutputEnableOverride(uint pin, uint value);

  /// Rejects an invalid GPIO number, including converted negative values.
  static void CheckPin(uint pin);

  /// Rejects masks that name a nonexistent RP2040 GPIO.
  static void CheckMask(uint64_t mask);

 private:
  friend FakePicoBoard& FakePico();

  FakePicoBoard() : gpio_(kGpioCount) {}

  enum class Attachment : uint8_t {
    kNone,
    kInput,
    kOutput,
    kIo,
    kUart,
    kSpi,
    kI2c
  };

  void checkAttachment(uint pin) const;

  void publish(uint32_t mask);

  template <typename Update>
  void updatePins(uint32_t mask, Update update);

  std::array<PinState, kGpioCount> pins_{};
  std::array<Attachment, kGpioCount> attachments_{};
  std::array<bool, kGpioCount> configured_{};
  std::array<roo_testing_transducers::VoltageSink*, kGpioCount> output_sinks_{};

  struct UartConnection {
    FakeUartDevice* device;
    int tx;
    int rx;
  };

  std::array<UartState, 2> uarts_{};
  std::vector<UartConnection> uart_connections_;
  struct SpiConnection {
    SimpleFakeSpiDevice* device;
    int clk;
    int miso;
    int mosi;
  };

  std::array<SpiState, 2> spis_{};
  std::vector<SpiConnection> spi_connections_;

  /// Rejects reconfiguration from within device callbacks or other transfers.
  void checkBusesIdle() const;

  /// Rejects changes to an active controller's pins or attached CS sink.
  void checkPinsIdle(uint32_t mask) const;

  struct I2cConnection {
    FakeI2cDevice* device;
    int sda;
    int scl;
  };

  std::array<I2cState, 2> i2cs_{};
  std::vector<I2cConnection> i2c_connections_;

  /// Computes SIO/PWM drive without consulting external voltage sources.
  std::optional<roo_testing_transducers::VoltageSignal> pinDrive(
      uint pin, const PinState& state) const;

  std::array<PwmState, 8> pwms_{};
  bool publishing_pwm_ = false;
  bool adc_enabled_ = false;
  uint adc_input_ = 0;
  std::array<bool, 4> adc_pins_{};
  FakeGpioInterface gpio_;
};

/// Returns the lazily constructed process-wide Pico board without hardware I/O.
FakePicoBoard& FakePico();
