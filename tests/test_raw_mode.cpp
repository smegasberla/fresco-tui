#include "harness.hpp"
#include "os_check.hpp"

#if OS_CHECK_UNIX
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "render.hpp"

namespace {

// Raw mode only makes sense on a terminal, so the test gives it a real one:
// a pty slave becomes STDIN while the test runs.
int open_pty_slave() {
  int master = posix_openpt(O_RDWR | O_NOCTTY);
  if (master < 0) return -1;
  if (grantpt(master) != 0 || unlockpt(master) != 0) {
    close(master);
    return -1;
  }
  // master is intentionally leaked: closing it would kill the slave.
  return open(ptsname(master), O_RDWR | O_NOCTTY);
}

struct StdinSwap {
  int saved;
  explicit StdinSwap(int fd) : saved(dup(STDIN_FILENO)) { dup2(fd, STDIN_FILENO); }
  ~StdinSwap() {
    dup2(saved, STDIN_FILENO);
    close(saved);
  }
};

} // namespace
#endif

TEST(raw_mode_turns_icanon_and_echo_off) {
#if OS_CHECK_UNIX
  int slave = open_pty_slave();
  if (slave < 0) SKIP("could not open a pty");

  {
    StdinSwap swap(slave);
    fresco::render::Screen screen;
    screen.enableRawMode();
  }

  struct termios t;
  CHECK(tcgetattr(slave, &t) == 0);
  CHECK((t.c_lflag & ICANON) == 0);
  CHECK((t.c_lflag & ECHO) == 0);
  close(slave);
#else
  SKIP("Pty test is POSIX-only");
#endif
}

TEST(raw_mode_restore_brings_icanon_and_echo_back) {
#if OS_CHECK_UNIX
  int slave = open_pty_slave();
  if (slave < 0) SKIP("could not open a pty");

  {
    StdinSwap swap(slave);
    fresco::render::Screen screen;
    screen.enableRawMode();
    screen.disableRawMode();
  }

  struct termios t;
  CHECK(tcgetattr(slave, &t) == 0);
  CHECK((t.c_lflag & ICANON) != 0);
  CHECK((t.c_lflag & ECHO) != 0);
  close(slave);
#else
  SKIP("Pty test is POSIX-only");
#endif
}

TEST(raw_mode_is_a_noop_without_a_terminal) {
#if OS_CHECK_UNIX
  // stdin is not a tty when tests run under make, so this must not crash.
  fresco::render::Screen screen;
  screen.enableRawMode();
  screen.disableRawMode();
#else
  SKIP("POSIX-only");
#endif
}
