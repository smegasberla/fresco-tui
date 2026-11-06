#include <string>

#include "harness.hpp"
#include "render.hpp"

// frame() builds the payload flush() writes, without touching the terminal, so
// everything about layout, styling and UTF-8 is checked here: no pty, and these
// tests run on every platform.

namespace {

using fresco::render::Screen;
using fresco::render::Style;

// Style with just a foreground color and one attribute, to keep the expected
// SGR strings short
Style red_bold() {
  Style s;
  s.fg = 0xFF0000;
  s.attrs = fresco::render::Bold;
  return s;
}

} // namespace

TEST(frame_of_an_untouched_screen_is_empty) {
  Screen screen; // resize() never ran: there is no frame to build

  CHECK(screen.frame().empty());
}

TEST(frame_emits_reset_a_move_per_row_and_every_cell) {
  Screen screen;
  screen.resize(2, 3);

  // RESET once for the frame pen, then one absolute move per row, the cells of
  // that row, and RESET at the end. Row 1 is last, so its bottom-right cell is
  // skipped: 2 cells instead of 3
  const std::string expected = "\033[0m"  // frame starts on the default pen
                               "\033[1;1H" // row 0
                               "   "       // 3 blank cells
                               "\033[2;1H" // row 1
                               "  "        // 2 cells: the last one is skipped
                               "\033[0m";  // pen back to default
  CHECK(screen.frame() == expected);

  // Building a frame must not consume anything: same cells, same bytes
  CHECK(screen.frame() == expected);
}

TEST(frame_emits_one_sgr_sequence_per_style_run) {
  Screen screen;
  screen.resize(1, 6);

  const Style red = red_bold();

  // Two calls, one style: the pen changes once, so the run costs one SGR
  // sequence and not one per cell
  screen.write(0, 0, "ab", red);
  screen.write(0, 2, "cd", red);

  const std::string expected = "\033[0m"                    // frame reset
                               "\033[1;1H"                   // row 0
                               "\033[0m\033[1;38;2;255;0;0m" // pen changes to red + bold
                               "abcd"                        // 4 cells, no further codes
                               "\033[0m"                     // pen back for the blank cell
                               " "                           // the 5th cell, default style
                               "\033[0m"; // end of frame; cell 6 is bottom-right, skipped
  CHECK(screen.frame() == expected);
}

TEST(frame_encodes_a_multibyte_code_point_as_utf8) {
  Screen screen;
  screen.resize(2, 2);

  screen.put(0, 0, U'\u2500', Style{}); // "─" is 3 bytes in UTF-8, one cell

  const std::string box = "\xE2\x94\x80";
  const std::string expected = "\033[0m\033[1;1H" + box + " " + "\033[2;1H" + " " + "\033[0m";
  CHECK(screen.frame() == expected);
}

TEST(frame_leaves_the_bottom_right_cell_alone) {
  Screen screen;
  screen.resize(1, 1); // a frame of exactly one cell: the one that gets skipped

  screen.put(0, 0, U'X', Style{});

  // The X must not appear: writing the last cell of the last row would wrap and
  // scroll the alt screen
  CHECK(screen.frame() == "\033[0m\033[1;1H\033[0m");
}

TEST(frame_draws_a_control_code_point_as_a_space) {
  Screen screen;
  screen.resize(2, 2);

  // put() takes any code point, so a raw ESC can reach the buffer. Emitting it
  // would start an escape sequence, so it becomes a space and keeps its column
  screen.put(0, 0, U'\033', Style{});

  CHECK(screen.frame() == "\033[0m\033[1;1H" "  " "\033[2;1H" " " "\033[0m");
}

TEST(frame_rebuilds_after_the_buffer_changes) {
  Screen screen;
  screen.resize(2, 2);

  screen.put(0, 0, U'A', Style{});
  CHECK(screen.frame().find('A') != std::string::npos);

  screen.put(0, 0, U'B', Style{});
  const std::string after = screen.frame();
  CHECK(after.find('B') != std::string::npos);
  CHECK(after.find('A') == std::string::npos);
}
