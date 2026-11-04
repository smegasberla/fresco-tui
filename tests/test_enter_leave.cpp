#include <cstdio>
#include <string>

#include "harness.hpp"
#include "os_check.hpp"

#if OS_CHECK_UNIX
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "render.hpp"

namespace {

// enter()/leave() talk to a real terminal, so each test hands them a pty:
// the slave becomes stdin and stdout while the call runs, and the bytes the
// call wrote are read back from the master.
struct CapturePty {
  int master = -1;
  int slave = -1;
  int saved_out = -1;
  int saved_in = -1;

  bool open() {
    master = posix_openpt(O_RDWR | O_NOCTTY);
    if (master < 0) return false;
    if (grantpt(master) != 0 || unlockpt(master) != 0) {
      close(master);
      master = -1;
      return false;
    }
    slave = ::open(ptsname(master), O_RDWR | O_NOCTTY);
    if (slave < 0) {
      close(master);
      master = -1;
      return false;
    }
    fcntl(master, F_SETFL, fcntl(master, F_GETFL) | O_NONBLOCK);
    return true;
  }

  void begin() {
    // Nothing may be left in the stdio buffer, or it would land in the capture.
    std::fflush(stdout);
    saved_out = dup(STDOUT_FILENO);
    saved_in = dup(STDIN_FILENO);
    dup2(slave, STDOUT_FILENO);
    dup2(slave, STDIN_FILENO);
  }

  void end() {
    std::fflush(stdout);
    dup2(saved_out, STDOUT_FILENO);
    dup2(saved_in, STDIN_FILENO);
    close(saved_out);
    close(saved_in);
    saved_out = saved_in = -1;
  }

  std::string read_all() {
    std::string out;
    char buf[512];
    for (int i = 0; i < 20; ++i) {
      ssize_t n = read(master, buf, sizeof buf);
      if (n > 0) {
        out.append(buf, static_cast<size_t>(n));
        continue;
      }
      break; // EAGAIN or EOF: nothing more was written
    }
    return out;
  }

  ~CapturePty() {
    if (slave >= 0) close(slave);
    if (master >= 0) close(master);
  }
};

// Prints the escaped bytes so a failure is readable.
void show_hex(const char* label, const std::string& s) {
  std::printf("    %s: ", label);
  for (unsigned char c : s) std::printf("\\x%02x", c);
  std::printf("\n");
}

} // namespace
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
