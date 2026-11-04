#include <cstdio>

#include "harness.hpp"
#include "render.hpp"

// These tests talk to the back buffer only: no pty, no terminal, no enter().

namespace {

using fresco::render::Cell;
using fresco::render::Screen;
using fresco::render::Style;

// Cell knows how to compare itself, and Cell{} is the blank cell, so this is
// just a name for "nothing written here yet"
bool is_blank(const Cell &c) { return c == Cell{}; }

// at() returns one shared blank object for every out-of-range position, so two
// clipped positions have the same address. That is the only way to see the
// bounds from outside without a writer.
bool clipped(const Screen &s, int row, int col) { return &s.at(row, col) == &s.at(-1, -1); }

} // namespace

TEST(cell_compares_character_and_style) {
  Cell blank;
  Cell other;
  CHECK(blank == other);

  other.cp = U'A';
  CHECK(!(blank == other));

  other = Cell{};
  other.s.attrs = fresco::render::Bold;
  CHECK(!(blank == other));

  other = Cell{};
  other.s.fg = 0x00FF00;
  CHECK(!(blank == other));
}

TEST(at_on_an_untouched_screen_returns_blank) {
  Screen screen; // never resized: the buffer is empty
  CHECK(is_blank(screen.at(0, 0)));
  CHECK(is_blank(screen.at(-1, -1)));
}

TEST(resize_blanks_every_cell) {
  Screen screen;
  screen.resize(24, 80);

  CHECK(is_blank(screen.at(0, 0)));
  CHECK(is_blank(screen.at(0, 79)));
  CHECK(is_blank(screen.at(23, 0)));
  CHECK(is_blank(screen.at(23, 79)));
}

TEST(at_out_of_range_returns_blank_not_garbage) {
  Screen screen;
  screen.resize(24, 80);

  CHECK(is_blank(screen.at(-1, 0)));  // above the buffer
  CHECK(is_blank(screen.at(24, 0)));  // one past the last row
  CHECK(is_blank(screen.at(0, -1)));  // left of the buffer
  CHECK(is_blank(screen.at(0, 80)));  // one past the last column
  CHECK(is_blank(screen.at(999, 999)));
}

TEST(rows_are_laid_out_row_major) {
  Screen screen;
  screen.resize(24, 80);

  const Cell *first = &screen.at(0, 0);

  // Next cell is one element later, next row is cols elements later
  CHECK(&screen.at(0, 1) == first + 1);
  CHECK(&screen.at(0, 79) == first + 79);
  CHECK(&screen.at(1, 0) == first + 80);
  CHECK(&screen.at(23, 79) == first + 24 * 80 - 1);
}

TEST(resize_with_the_same_size_does_not_reallocate) {
  Screen screen;
  screen.resize(24, 80);

  const Cell *before = &screen.at(0, 0);
  screen.resize(24, 80);
  const Cell *after = &screen.at(0, 0);

  // Same address means no reallocation, which is what makes calling resize()
  // every frame cheap
  CHECK(before == after);
}

TEST(resize_changes_which_cells_are_in_range) {
  Screen screen;
  screen.resize(24, 80);
  CHECK(!clipped(screen, 15, 15));

  screen.resize(10, 10);

  CHECK(!clipped(screen, 9, 9));
  CHECK(clipped(screen, 10, 10));  // one past the last row and column
  CHECK(clipped(screen, 15, 15));  // was inside before the shrink
  CHECK(is_blank(screen.at(15, 15))); // and reading it is still safe
}

TEST(resize_after_a_shrink_does_not_resurrect_content) {
  Screen screen;
  screen.resize(24, 80);
  screen.resize(2, 2);
  screen.resize(24, 80);

  CHECK(is_blank(screen.at(0, 0)));
  CHECK(is_blank(screen.at(23, 79)));
}

TEST(put_then_at_round_trips_character_and_style) {
  Screen screen;
  screen.resize(24, 80);

  Style green_bold;
  green_bold.fg = 0x00FF00;
  green_bold.attrs = fresco::render::Bold;

  screen.put(5, 7, U'A', green_bold);

  const Cell &c = screen.at(5, 7);
  CHECK(c.cp == U'A');
  CHECK(c.s.fg == 0x00FF00);
  CHECK(c.s.attrs == fresco::render::Bold);
  CHECK(!(c == Cell{}));
}

TEST(put_does_not_touch_the_neighbours) {
  Screen screen;
  screen.resize(24, 80);

  screen.put(1, 1, U'X', Style{});

  CHECK(screen.at(1, 1).cp == U'X');
  CHECK(is_blank(screen.at(0, 1)));
  CHECK(is_blank(screen.at(1, 0)));
  CHECK(is_blank(screen.at(1, 2)));
  CHECK(is_blank(screen.at(2, 1)));
}

TEST(put_overwrites_what_was_there) {
  Screen screen;
  screen.resize(24, 80);

  screen.put(0, 0, U'A', Style{});
  screen.put(0, 0, U'B', Style{});

  CHECK(screen.at(0, 0).cp == U'B');
}

TEST(put_and_at_clip_at_the_same_edges) {
  Screen screen;
  screen.resize(2, 2);

  // One past the last column of row 0: with a > instead of >= this lands on
  // (1, 0), so the neighbour check is what catches the divergence
  screen.put(0, 2, U'X', Style{});
  CHECK(is_blank(screen.at(0, 0)));
  CHECK(is_blank(screen.at(1, 0)));

  // One past the last row
  screen.put(2, 0, U'X', Style{});
  CHECK(is_blank(screen.at(0, 0)));
  CHECK(is_blank(screen.at(1, 1)));

  // Negative coordinates must not become huge size_t offsets
  screen.put(-1, 0, U'X', Style{});
  screen.put(0, -1, U'X', Style{});

  for (int r = 0; r < 2; ++r)
    for (int c = 0; c < 2; ++c) CHECK(is_blank(screen.at(r, c)));
}

TEST(put_without_a_resize_writes_nothing) {
  Screen screen; // buffer never allocated
  screen.put(0, 0, U'A', Style{});
  CHECK(is_blank(screen.at(0, 0)));
}

TEST(put_then_shrink_then_grow_leaves_blanks) {
  Screen screen;
  screen.resize(4, 4);
  screen.put(3, 3, U'Z', Style{});
  CHECK(screen.at(3, 3).cp == U'Z');

  screen.resize(2, 2);
  screen.resize(4, 4);

  CHECK(is_blank(screen.at(3, 3)));
}

TEST(resize_with_negative_or_zero_sizes_gives_an_empty_buffer) {
  Screen screen;

  screen.resize(-5, 10);
  CHECK(is_blank(screen.at(0, 0)));

  screen.resize(10, -5);
  CHECK(is_blank(screen.at(0, 0)));

  screen.resize(0, 0);
  CHECK(is_blank(screen.at(0, 0)));

  // And it still works afterwards
  screen.resize(3, 3);
  CHECK(is_blank(screen.at(2, 2)));
  CHECK(is_blank(screen.at(3, 3)));
}
