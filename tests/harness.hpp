#pragma once

// Minimal test harness: no dependencies, one binary, one line per test.
//   TEST(name) { CHECK(expr); }
// The runner lives in tests/main.cpp, so every test file is just TEST blocks.

#include <cstdio>
#include <vector>

namespace fresco::test {

struct Skipped {};

struct Case {
  const char *name;
  void (*fn)();
};

inline std::vector<Case> &cases() {
  static std::vector<Case> all;
  return all;
}

struct Register {
  Register(const char *name, void (*fn)()) { cases().push_back({name, fn}); }
};

[[noreturn]] inline void fail(const char *file, int line, const char *expr) {
  std::printf("    %s:%d: CHECK failed: %s\n", file, line, expr);
  throw 1; // caught by the runner as a failed test
}

} // namespace fresco::test

#define TEST(name)                                                                                 \
  static void name();                                                                              \
  static ::fresco::test::Register reg_##name(#name, name);                                         \
  static void name()

#define CHECK(cond)                                                                                \
  do {                                                                                             \
    if (!(cond)) ::fresco::test::fail(__FILE__, __LINE__, #cond);                                  \
  } while (0)

#define SKIP(why)                                                                                  \
  do {                                                                                             \
    std::printf("    skipped: %s\n", why);                                                         \
    throw ::fresco::test::Skipped{};                                                               \
  } while (0)
