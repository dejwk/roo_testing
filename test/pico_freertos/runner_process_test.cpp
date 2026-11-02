#include <sys/wait.h>
#include <unistd.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

namespace {
// Starts a fresh scheduler process so each run also validates native cleanup.
int Run(const char* executable, const char* filter) {
  const pid_t child = fork();
  if (child == 0) {
    execl(executable, executable, "--gtest_also_run_disabled_tests", filter,
          static_cast<char*>(nullptr));
    std::_Exit(127);
  }
  if (child < 0) return -1;
  int status;
  if (waitpid(child, &status, 0) != child) return -1;
  return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}
}  // namespace

// Verifies repeated startup/cleanup, exact GoogleTest exit codes, and explicit
// rejection of unsupported core-1 entry points in separate native processes.
int main(int argc, char** argv) {
  if (argc != 5) return 2;
  for (int repeat = 0; repeat < 3; ++repeat) {
    for (int i : {1, 2}) {
      if (Run(argv[i], "--gtest_filter=-*DISABLED*") != 0) return 3;
      if (Run(argv[i], "--gtest_filter=*DISABLED_ExpectedFailure") != 1)
        return 4;
    }
    if (Run(argv[4], "--gtest_filter=-*DISABLED*") != 0) return 5;
    if (Run(argv[4], "--gtest_filter=*DISABLED_PropagatesFailure") != 1)
      return 6;
  }
  const int core1 = Run(argv[3], "--iterations=1");
  if (core1 != 128 + SIGABRT) return 7;
  std::puts("Pico FreeRTOS runners and lifecycle verified");
  return 0;
}
