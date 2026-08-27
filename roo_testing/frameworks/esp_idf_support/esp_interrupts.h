#pragma once

#include <cstddef>
#include <cstdint>

namespace roo_testing::esp_idf {

/// One materialized target-register value associated with a source assertion.
struct InterruptStatusSnapshot {
  /// Numeric target address used when the handler was registered.
  uint32_t target_address;

  /// Register value materialized by the emulated peripheral.
  uint32_t value;
};

/// Raises a logical interrupt source without target-register status.
///
/// Source values remain signed and target-defined. Unconditional handlers are
/// eligible, while shared handlers registered with an interrupt-status filter
/// are skipped because their target status is unknown. This lock-free
/// operation may be called from a native host thread or an emulated ISR.
void raiseInterruptSource(int source) noexcept;

/// Raises a logical interrupt source with materialized target status.
///
/// The snapshots are borrowed only for this call and target addresses are
/// treated as opaque numeric keys; they are never dereferenced on the host.
/// Matching shared handlers are selected before the FreeRTOS task is
/// interrupted. Multiple assertions may coalesce.
void raiseInterruptSource(int source, const InterruptStatusSnapshot* snapshots,
                          size_t snapshot_count) noexcept;

}  // namespace roo_testing::esp_idf
