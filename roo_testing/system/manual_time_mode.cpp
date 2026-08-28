#include <csignal>

extern "C" volatile sig_atomic_t system_time_auto_sync_mode;
volatile sig_atomic_t system_time_auto_sync_mode = 0;
