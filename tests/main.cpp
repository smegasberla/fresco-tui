#include <cstdio>
#include <cstring>

#include "harness.hpp"

// Usage: ./build/tests/run            run everything
//        ./build/tests/run raw_mode   run tests whose name contains "raw_mode"

int main(int argc, char **argv) {
  int ran = 0, failed = 0, skipped = 0;
  const char *filter = argc > 1 ? argv[1] : nullptr;

  for (const auto &c : fresco::test::cases()) {
    if (filter && std::strstr(c.name, filter) == nullptr) continue;

    ++ran;
    std::printf("[ RUN  ] %s\n", c.name);
    try {
      c.fn();
      std::printf("[  OK  ] %s\n", c.name);
    } catch (const fresco::test::Skipped &) {
      ++skipped;
      std::printf("[ SKIP ] %s\n", c.name);
    } catch (...) {
      ++failed;
      std::printf("[ FAIL ] %s\n", c.name);
    }
  }

  std::printf("\n%d run, %d failed, %d skipped\n", ran, failed, skipped);
  return failed == 0 ? 0 : 1;
}
