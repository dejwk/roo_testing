#pragma once

// Destination-owned native API overlay for Pico SDK 2.3.1. The pinned SDK has
// no host hardware_spi module; device register headers are intentionally
// absent.
#include "pico.h"
#include "pico/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Identifies a checked native SPI controller without exposing registers.
typedef struct spi_inst spi_inst_t;

/// Names controller 0.
extern spi_inst_t* const spi0;

/// Names controller 1.
extern spi_inst_t* const spi1;

#if PICO_DEFAULT_SPI == 0
#define spi_default spi0
#else
#define spi_default spi1
#endif

/// Selects idle clock polarity.
typedef enum { SPI_CPOL_0 = 0, SPI_CPOL_1 = 1 } spi_cpol_t;

/// Selects the sampling clock phase.
typedef enum { SPI_CPHA_0 = 0, SPI_CPHA_1 = 1 } spi_cpha_t;

/// Selects bit order; the PL022 and this adapter support MSB first only.
typedef enum { SPI_LSB_FIRST = 0, SPI_MSB_FIRST = 1 } spi_order_t;

/// Returns the checked controller number, 0 or 1.
uint spi_get_index(const spi_inst_t* spi);

/// Returns the checked controller instance.
spi_inst_t* spi_get_instance(uint num);

/// Initializes an enabled mode-0, 8-bit master; returns achieved baud.
uint spi_init(spi_inst_t* spi, uint baudrate);

/// Disables the controller while preserving fixture wiring and pin muxes.
void spi_deinit(spi_inst_t* spi);

/// Selects a pinned-SDK divider at nominal 125 MHz; returns achieved baud.
uint spi_set_baudrate(spi_inst_t* spi, uint baudrate);

/// Returns the achieved baudrate of an enabled controller.
uint spi_get_baudrate(const spi_inst_t* spi);

/// Configures 8/16-bit MSB-first frames and clock mode; rejects other formats.
void spi_set_format(spi_inst_t* spi, uint data_bits, spi_cpol_t cpol,
                    spi_cpha_t cpha, spi_order_t order);

/// Keeps master operation when false; slave mode fails explicitly.
void spi_set_slave(spi_inst_t* spi, bool slave);

/// Reports a synchronous transfer currently executing on the native controller.
bool spi_is_busy(const spi_inst_t* spi);

/// Rejects unsupported hardware FIFO status queries.
bool spi_is_writable(const spi_inst_t* spi);

/// Rejects unsupported hardware FIFO status queries.
bool spi_is_readable(const spi_inst_t* spi);

/// Exchanges len 8-bit frames; returns len or a negative PICO_ERROR code.
/// Invalid arguments/routes leave buffers, device state, and time untouched.
/// Identical src/dst is supported; partial overlap is rejected.
int spi_write_read_blocking(spi_inst_t* spi, const uint8_t* src, uint8_t* dst,
                            size_t len);

/// Sends len 8-bit frames, discarding incoming data; returns len or PICO_ERROR.
int spi_write_blocking(spi_inst_t* spi, const uint8_t* src, size_t len);

/// Receives len 8-bit frames while transmitting repeated_tx_data.
/// Returns len or a negative PICO_ERROR code.
int spi_read_blocking(spi_inst_t* spi, uint8_t repeated_tx_data, uint8_t* dst,
                      size_t len);

/// Exchanges len 16-bit frames, high byte first; returns len or PICO_ERROR.
int spi_write16_read16_blocking(spi_inst_t* spi, const uint16_t* src,
                                uint16_t* dst, size_t len);

/// Sends len 16-bit frames, high byte first; returns len or PICO_ERROR.
int spi_write16_blocking(spi_inst_t* spi, const uint16_t* src, size_t len);

/// Receives len 16-bit frames while transmitting repeated_tx_data.
/// Returns len or a negative PICO_ERROR code.
int spi_read16_blocking(spi_inst_t* spi, uint16_t repeated_tx_data,
                        uint16_t* dst, size_t len);

#ifdef __cplusplus
}
#endif
