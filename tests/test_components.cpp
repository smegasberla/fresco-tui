#include "harness.hpp"
#include "components.hpp"
#include "render.hpp"

// Components only write into the back buffer, so their tests need no pty: the
// cells they produce are read back with at().

TEST(button_draws_its_text_at_its_own_position) {
  fresco::render::Screen screen;
  screen.resize(4, 20);

  // Button(text, x, y, height, width): x is the column, y is the row
  fresco::component::Button button("ok", 3, 1, 1, 4);
  button.draw(screen);

  CHECK(screen.at(1, 3).cp == U'o');
  CHECK(screen.at(1, 4).cp == U'k');
  CHECK(screen.at(1, 5).cp == U' '); // nothing past the text
  CHECK(screen.at(0, 3).cp == U' '); // and nothing on the row above
}

TEST(button_geometry_is_the_size_it_was_given) {
  fresco::render::Screen screen;
  screen.resize(4, 20);

  // The x and y arguments must land in the base class: reading the base members
  // instead would put every button at 0,0
  fresco::component::Button far("x", 7, 2, 1, 1);
  far.draw(screen);

  CHECK(screen.at(2, 7).cp == U'x');
  CHECK(screen.at(0, 0).cp == U' ');
  CHECK(screen.at(2, 0).cp == U' ');
}

TEST(button_text_is_clipped_to_its_width) {
  fresco::render::Screen screen;
  screen.resize(2, 10);

  // The box is 2 cells wide, so a 4 letter label cannot all fit. With the width
  // never reaching the base class every label would print whole
  fresco::component::Button tight("abcd", 0, 0, 1, 2);
  tight.draw(screen);

  CHECK(screen.at(0, 0).cp == U'a');
  CHECK(screen.at(0, 1).cp == U'b');
  CHECK(screen.at(0, 2).cp == U' ');
}

TEST(button_text_is_clipped_by_the_screen) {
  fresco::render::Screen screen;
  screen.resize(2, 4);

  // Starts at the last column: only "a" fits, the rest is dropped by write()
  fresco::component::Button long_text("abcd", 3, 0, 1, 4);
  long_text.draw(screen);

  CHECK(screen.at(0, 3).cp == U'a');
  CHECK(screen.at(1, 3).cp == U' '); // no wrap onto the next row
}

TEST(button_without_text_draws_nothing) {
  fresco::render::Screen screen;
  screen.resize(2, 4);

  fresco::component::Button empty;
  empty.draw(screen);

  for (int row = 0; row < 2; ++row)
    for (int col = 0; col < 4; ++col) CHECK(screen.at(row, col) == fresco::render::Cell{});
}
