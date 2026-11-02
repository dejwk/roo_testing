#pragma once
namespace roo_testing {
namespace freertos_posix {
/// Runs @p body in a FreeRTOS task and returns its result to native main.
/// The body borrows @p context until completion. One scheduler lifetime is
/// supported per process. Remaining tasks must own no controller reservation.
int Run(int (*body)(void*), void* context = nullptr);
}  // namespace freertos_posix
}  // namespace roo_testing
