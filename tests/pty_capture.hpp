#pragma once

// A pty around one call, so a test can read back the exact bytes a function
// wrote. enter()/leave()/clearScreen()/flush() all speak to a real terminal, so
// each test swaps the slave in as stdin and stdout, runs the call, and reads
// the payload from the master afterwards.
//
// POSIX only: on Windows the terminal calls go through the console API, which
// has no pty to hand out, so tests including this header SKIP there.

#include <cstdio>
#include <string>

#include "os_check.hpp"
#include "render.hpp"

#if OS_CHECK_UNIX
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

namespace fresco::test {

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
inline void show_hex(const char *label, const std::string &s) {
  std::printf("    %s: ", label);
  for (unsigned char c : s) std::printf("\\x%02x", c);
  std::printf("\n");
}

// Runs one call against a pty and returns the bytes it wrote. The screen goes
// in already entered, and everything enter() itself wrote is dropped first.
template <typename Fn> std::string capture_after_enter(fresco::render::Screen &screen, Fn call) {
  CapturePty t;
  if (!t.open()) return std::string();

  {
    t.begin();
    screen.enter();
    t.end();
  }
  t.read_all(); // drop the enter payload

  {
    t.begin();
    call(screen);
    t.end();
  }

  return t.read_all();
}

} // namespace fresco::test
#endif
