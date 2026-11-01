#pragma once

#include "pico.h"
#include "pico/error.h"
#include "pico/time.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Names a checked native I2C controller, without device registers.
typedef struct i2c_inst i2c_inst_t;

/// Selects controller 0.
extern i2c_inst_t* const i2c0;

/// Selects controller 1.
extern i2c_inst_t* const i2c1;

#if PICO_DEFAULT_I2C == 0
#define i2c_default i2c0
#else
#define i2c_default i2c1
#endif

/// Checks identity and returns controller number 0 or 1.
uint i2c_get_index(i2c_inst_t* i2c);

/// Returns the checked controller instance.
i2c_inst_t* i2c_get_instance(uint num);

/// Initializes an enabled seven-bit master at the achieved nominal-clock baud.
uint i2c_init(i2c_inst_t* i2c, uint baudrate);

/// Disables the controller and clears retained repeated-START intent.
void i2c_deinit(i2c_inst_t* i2c);

/// Uses the pinned SDK clock-count rounding at nominal 125 MHz.
uint i2c_set_baudrate(i2c_inst_t* i2c, uint baudrate);

/// Retains master mode for false; slave mode fails explicitly.
void i2c_set_slave_mode(i2c_inst_t* i2c, bool slave, uint8_t addr);

/// Transfers 1..65535 bytes; nostop preserves the next repeated-START intent.
/// Returns len on success or PICO_ERROR; oversize and invalid requests have no
/// device effects. Blocking calls forward the maximum model timeout (65535 ms).
int i2c_write_blocking(i2c_inst_t* i2c, uint8_t addr, const uint8_t* src,
                       size_t len, bool nostop);

/// Receives bytes atomically into dst on success; errors leave dst unchanged.
int i2c_read_blocking(i2c_inst_t* i2c, uint8_t addr, uint8_t* dst, size_t len,
                      bool nostop);

/// Applies a relative timeout, rounded up and saturated to model milliseconds.
int i2c_write_timeout_us(i2c_inst_t* i2c, uint8_t addr, const uint8_t* src,
                         size_t len, bool nostop, uint timeout_us);

/// Applies a relative timeout to a read. Zero expires without device delivery.
int i2c_read_timeout_us(i2c_inst_t* i2c, uint8_t addr, uint8_t* dst, size_t len,
                        bool nostop, uint timeout_us);

/// Applies the remaining shared-clock time until an absolute deadline.
int i2c_write_blocking_until(i2c_inst_t* i2c, uint8_t addr, const uint8_t* src,
                             size_t len, bool nostop, absolute_time_t until);

/// Reads with an absolute shared-clock deadline; expired calls do no I/O.
int i2c_read_blocking_until(i2c_inst_t* i2c, uint8_t addr, uint8_t* dst,
                            size_t len, bool nostop, absolute_time_t until);

/// Approximates per-character timeout by forwarding the complete call budget.
int i2c_write_timeout_per_char_us(i2c_inst_t* i2c, uint8_t addr,
                                  const uint8_t* src, size_t len, bool nostop,
                                  uint timeout_us);

/// Approximates per-character read timeout by forwarding the complete budget.
int i2c_read_timeout_per_char_us(i2c_inst_t* i2c, uint8_t addr, uint8_t* dst,
                                 size_t len, bool nostop, uint timeout_us);

#ifdef __cplusplus
}
#endif
