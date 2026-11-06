#include <cstdio>
#include <string_view>

#include "harness.hpp"
#include "render.hpp"

// write() is buffer-only: no pty, no terminal, no enter(). Every case below
// checks the cells with at(), which is the only reader of the back buffer.

namespace {

using fresco::render::Cell;
using fresco::render::Screen;
using fresco::render::Style;

// Cell{} is the blank cell, so "nothing was written here" is just equality
bool is_blank(const Cell &c) { return c == Cell{}; }

// The cells of one row, as code points, for the "did the text land where it
// should" cases. Reads through at(), so an off-buffer cell reads as a blank
bool row_is(const Screen &s, int row, int col, std::string_view expected) {
  // expected is plain ASCII here on purpose: the multi-byte cases compare one
  // code point at a time, so no second UTF-8 decoder is needed in the tests
  for (std::size_t i = 0; i < expected.size(); ++i) {
    if (s.at(row, col + static_cast<int>(i)).cp != static_cast<char32_t>(expected[i]))
      return false;
  }
  return true;
}

} // namespace

TEST(write_stores_the_text_at_the_given_position) {
  Screen screen;
  screen.resize(24, 80);

  screen.write(5, 7, "abc", Style{});

  CHECK(screen.at(5, 7).cp == U'a');
  CHECK(screen.at(5, 8).cp == U'b');
  CHECK(screen.at(5, 9).cp == U'c');
  CHECK(row_is(screen, 5, 7, "abc"));
}

TEST(write_applies_one_style_to_every_cell_it_writes) {
  Screen screen;
  screen.resize(24, 80);

  Style green_bold;
  green_bold.fg = 0x00FF00;
  green_bold.attrs = fresco::render::Bold;

  screen.write(2, 2, "hi", green_bold);

  for (int col = 2; col < 4; ++col) {
    CHECK(screen.at(2, col).s == green_bold);
    CHECK(screen.at(2, col).s.fg == 0x00FF00);
    CHECK(screen.at(2, col).s.attrs == fresco::render::Bold);
  }

  // The cell right after the text keeps the default style
  CHECK(screen.at(2, 4).s == Style{});
}

TEST(write_does_not_touch_the_neighbours) {
  Screen screen;
  screen.resize(24, 80);

  screen.write(1, 1, "ab", Style{});

  CHECK(is_blank(screen.at(0, 1)));  // row above
  CHECK(is_blank(screen.at(2, 1)));  // row below
  CHECK(is_blank(screen.at(1, 0)));  // before the text
  CHECK(is_blank(screen.at(1, 3)));  // right after the text
}

TEST(write_decodes_three_and_two_byte_utf8) {
  Screen screen;
  screen.resize(24, 80);

  // "─" is U+2500 (3 bytes), "é" is U+00E9 (2 bytes). Two code points, so two
  // columns: a byte-per-cell bug would leave 5 cells and skip the rest
  screen.write(0, 0, "\u2500\u00e9", Style{});

  CHECK(screen.at(0, 0).cp == 0x2500);
  CHECK(screen.at(0, 1).cp == 0x00E9);
  CHECK(is_blank(screen.at(0, 2)));
}

TEST(write_decodes_four_byte_utf8) {
  Screen screen;
  screen.resize(24, 80);

  screen.write(0, 0, "\U0001f600", Style{}); // 😀 = U+1F600

  CHECK(screen.at(0, 0).cp == 0x1F600);
  CHECK(is_blank(screen.at(0, 1)));
}

TEST(write_keeps_the_cell_run_after_a_multibyte_character) {
  Screen screen;
  screen.resize(24, 80);

  // The column counter must advance per code point, not per byte
  screen.write(3, 4, "a\u2500b", Style{});

  CHECK(screen.at(3, 4).cp == U'a');
  CHECK(screen.at(3, 5).cp == 0x2500);
  CHECK(screen.at(3, 6).cp == U'b');
  CHECK(is_blank(screen.at(3, 7)));
}

TEST(write_does_not_wrap_at_the_right_edge) {
  Screen screen;
  screen.resize(3, 10);

  // Starts at the second to last column, so only two characters fit
  screen.write(1, 8, "abcd", Style{});

  CHECK(screen.at(1, 8).cp == U'a');
  CHECK(screen.at(1, 9).cp == U'b');

  // The overflow must not appear on the next row
  CHECK(is_blank(screen.at(2, 0)));
  CHECK(is_blank(screen.at(2, 1)));
  CHECK(is_blank(screen.at(0, 0)));
}

TEST(write_drops_a_row_outside_the_buffer) {
  Screen screen;
  screen.resize(3, 10);

  screen.write(3, 0, "zzz", Style{});   // one past the last row
  screen.write(999, 0, "zzz", Style{}); // far outside
  screen.write(-1, 0, "zzz", Style{});  // above the buffer

  for (int row = 0; row < 3; ++row)
    for (int col = 0; col < 10; ++col) CHECK(is_blank(screen.at(row, col)));
}

TEST(write_with_a_negative_column_keeps_the_visible_tail) {
  Screen screen;
  screen.resize(3, 10);

  // "x" lands off the left edge, "y" lands on column 0. put() clips each cell,
  // so the column counter has to keep counting through the clipped one
  screen.write(0, -1, "xy", Style{});

  CHECK(screen.at(0, 0).cp == U'y');
  CHECK(is_blank(screen.at(0, 1)));
}

TEST(write_of_an_empty_string_writes_nothing) {
  Screen screen;
  screen.resize(3, 10);

  screen.write(0, 0, "", Style{});

  for (int col = 0; col < 10; ++col) CHECK(is_blank(screen.at(0, col)));
}

TEST(write_without_a_resize_writes_nothing) {
  Screen screen; // buffer never allocated, rows and cols are 0

  screen.write(0, 0, "hello", Style{});

  CHECK(is_blank(screen.at(0, 0)));
}

TEST(write_overwrites_the_cells_it_covers) {
  Screen screen;
  screen.resize(3, 10);

  screen.write(0, 0, "abcdef", Style{});
  screen.write(0, 2, "XY", Style{});

  CHECK(row_is(screen, 0, 0, "abXYef"));
}

TEST(write_skips_control_characters_without_taking_a_column) {
  Screen screen;
  screen.resize(3, 10);

  // \t, \n and \033 would move the cursor or start an escape sequence during
  // flush(), so they never reach a cell and never consume a column
  screen.write(0, 0, "a\tb\nd\re", Style{});

  CHECK(row_is(screen, 0, 0, "abde"));
  CHECK(is_blank(screen.at(0, 4)));
}

TEST(write_neutralises_an_escape_sequence) {
  Screen screen;
  screen.resize(3, 10);

  // The ESC itself is dropped, so nothing can be injected into the terminal.
  // The printable tail ("[31m") is ordinary text and does take columns
  screen.write(0, 0, "\033[31mx", Style{});

  CHECK(screen.at(0, 0).cp == U'[');
  CHECK(screen.at(0, 1).cp == U'3');
  CHECK(screen.at(0, 2).cp == U'1');
  CHECK(screen.at(0, 3).cp == U'm');
  CHECK(screen.at(0, 4).cp == U'x');
}

TEST(write_stops_at_a_truncated_sequence) {
  Screen screen;
  screen.resize(3, 10);

  // Lead byte promises 3 bytes, only 1 continuation is there: stop instead of
  // reading past the end of the view
  screen.write(0, 0, "ab\xE2\x94", Style{});

  CHECK(row_is(screen, 0, 0, "ab"));
  CHECK(is_blank(screen.at(0, 2)));
}

TEST(write_stops_at_an_invalid_lead_byte) {
  Screen screen;
  screen.resize(3, 10);

  // 0xFF is not ASCII, not a lead byte, and not a continuation byte
  screen.write(0, 0, "a\xFFz", Style{});

  CHECK(row_is(screen, 0, 0, "a"));
  CHECK(is_blank(screen.at(0, 1)));
}

TEST(write_stops_at_a_bad_continuation_byte) {
  Screen screen;
  screen.resize(3, 10);

  // ASCII right after a lead byte: the promised length was wrong
  screen.write(0, 0, "a\xE2zb", Style{});

  CHECK(row_is(screen, 0, 0, "a"));
  CHECK(is_blank(screen.at(0, 1)));
}

TEST(write_is_independent_of_the_row_length) {
  Screen screen;
  screen.resize(3, 10);

  // Same text on two rows: the second one must not see anything from the first
  screen.write(0, 0, "hello", Style{});
  screen.write(1, 0, "hi", Style{});

  CHECK(row_is(screen, 0, 0, "hello"));
  CHECK(row_is(screen, 1, 0, "hi"));
  CHECK(is_blank(screen.at(1, 2)));
}
