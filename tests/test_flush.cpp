#include <cstdio>
#include <string>

#include "harness.hpp"
#include "os_check.hpp"
#include "pty_capture.hpp"
#include "render.hpp"

// flush() is the write half: frame() builds the bytes (see test_frame.cpp), and
// flush() pushes them to stdout. Only that last step needs a pty, so the tests
// here are about when it writes and not what it writes.

#if OS_CHECK_UNIX
namespace {
using fresco::render::Screen;
using fresco::render::Style;
} // namespace
#endif

TEST(flush_without_enter_emits_nothing) {
#if OS_CHECK_UNIX
  fresco::test::CapturePty t;
  if (!t.open()) SKIP("could not open a pty");

  Screen screen;
  screen.resize(2, 2); // a real frame, but no alt screen to draw it on

  {
    t.begin();
    screen.flush();
    t.end();
  }

  std::string got = t.read_all();
  if (!got.empty()) fresco::test::show_hex("got", got);
  CHECK(got.empty());
#else
  SKIP("Pty test is POSIX-only");
#endif
}

TEST(flush_without_a_frame_emits_nothing) {
#if OS_CHECK_UNIX
  Screen screen;
  screen.enter(); // entered, but resize() never ran

  std::string got = fresco::test::capture_after_enter(screen, [](Screen &s) { s.flush(); });

  if (!got.empty()) fresco::test::show_hex("got", got);
  CHECK(got.empty());
#else
  SKIP("Pty test is POSIX-only");
#endif
}

TEST(flush_writes_exactly_the_frame) {
#if OS_CHECK_UNIX
  Screen screen;
  screen.enter();

  std::string want;
  std::string got = fresco::test::capture_after_enter(screen, [&want](Screen &s) {
    s.resize(2, 4);
    Style red;
    red.fg = 0xFF0000;
    s.write(0, 1, "hi", red);
    want = s.frame(); // built without touching the terminal
    s.flush();
  });

  if (got != want) {
    fresco::test::show_hex("got     ", got);
    fresco::test::show_hex("expected", want);
  }
  CHECK(!want.empty());
  CHECK(got == want);
#else
  SKIP("Pty test is POSIX-only");
#endif
}
