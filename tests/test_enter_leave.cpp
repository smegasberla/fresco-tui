#include <cstdio>
#include <string>

#include "harness.hpp"
#include "os_check.hpp"
#include "pty_capture.hpp"
#include "render.hpp"

#if OS_CHECK_UNIX
using fresco::test::CapturePty;
using fresco::test::show_hex;
#endif

TEST(enter_emits_alt_screen_home_reset_and_hide_cursor) {
#if OS_CHECK_UNIX
  CapturePty t;
  if (!t.open()) SKIP("could not open a pty");

  {
    t.begin();
    fresco::render::Screen screen;
    screen.enter();
    t.end();
  }

  const std::string expected = "\033[?1049h\033[H\033[0m\033[?25l";
  std::string got = t.read_all();
  if (got != expected) {
    show_hex("got     ", got);
    show_hex("expected", expected);
  }
  CHECK(got == expected);
#else
  SKIP("Pty test is POSIX-only");
#endif
}

TEST(leave_emits_reset_show_cursor_and_leave_alt_screen) {
#if OS_CHECK_UNIX
  CapturePty t;
  if (!t.open()) SKIP("could not open a pty");

  fresco::render::Screen screen;

  {
    t.begin();
    screen.enter();
    t.end();
  }
  t.read_all(); // drop the enter payload

  {
    t.begin();
    screen.leave();
    t.end();
  }

  const std::string expected = "\033[0m\033[?25h\033[?1049l";
  std::string got = t.read_all();
  if (got != expected) {
    show_hex("got     ", got);
    show_hex("expected", expected);
  }
  CHECK(got == expected);
#else
  SKIP("Pty test is POSIX-only");
#endif
}

TEST(leave_without_enter_emits_nothing) {
#if OS_CHECK_UNIX
  CapturePty t;
  if (!t.open()) SKIP("could not open a pty");

  fresco::render::Screen screen; // never entered

  {
    t.begin();
    screen.leave();
    t.end();
  }

  std::string got = t.read_all();
  if (!got.empty()) show_hex("got", got);
  CHECK(got.empty());
#else
  SKIP("Pty test is POSIX-only");
#endif
}

TEST(enter_twice_emits_one_payload) {
#if OS_CHECK_UNIX
  CapturePty t;
  if (!t.open()) SKIP("could not open a pty");

  {
    t.begin();
    fresco::render::Screen screen;
    screen.enter();
    screen.enter(); // must be a no-op
    t.end();
  }

  const std::string expected = "\033[?1049h\033[H\033[0m\033[?25l";
  std::string got = t.read_all();
  if (got != expected) {
    show_hex("got     ", got);
    show_hex("expected", expected);
  }
  CHECK(got == expected);
#else
  SKIP("Pty test is POSIX-only");
#endif
}

TEST(second_screen_cannot_leave_the_first) {
#if OS_CHECK_UNIX
  CapturePty t;
  if (!t.open()) SKIP("could not open a pty");

  fresco::render::Screen a;
  fresco::render::Screen b;

  {
    t.begin();
    a.enter();
    t.end();
  }
  t.read_all();

  {
    t.begin();
    b.leave(); // b never entered: must not touch a's terminal
    t.end();
  }

  std::string got = t.read_all();
  if (!got.empty()) show_hex("got", got);
  CHECK(got.empty());

  {
    t.begin();
    a.leave();
    t.end();
  }
  CHECK(t.read_all() == "\033[0m\033[?25h\033[?1049l");
#else
  SKIP("Pty test is POSIX-only");
#endif
}

TEST(clear_screen_emits_reset_clear_and_home) {
#if OS_CHECK_UNIX
  CapturePty t;
  if (!t.open()) SKIP("could not open a pty");

  fresco::render::Screen screen;

  {
    t.begin();
    screen.enter();
    t.end();
  }
  t.read_all(); // drop the enter payload

  {
    t.begin();
    screen.clearScreen();
    t.end();
  }

  // RESET first (so background-color-erase does not paint the screen),
  // then the erase, then home (erasing does not move the cursor)
  const std::string expected = "\033[0m\033[2J\033[H";
  std::string got = t.read_all();
  if (got != expected) {
    show_hex("got     ", got);
    show_hex("expected", expected);
  }
  CHECK(got == expected);
#else
  SKIP("Pty test is POSIX-only");
#endif
}

TEST(clear_screen_without_enter_emits_nothing) {
#if OS_CHECK_UNIX
  CapturePty t;
  if (!t.open()) SKIP("could not open a pty");

  fresco::render::Screen screen; // never entered

  {
    t.begin();
    screen.clearScreen();
    t.end();
  }

  std::string got = t.read_all();
  if (!got.empty()) show_hex("got", got);
  CHECK(got.empty());
#else
  SKIP("Pty test is POSIX-only");
#endif
}
