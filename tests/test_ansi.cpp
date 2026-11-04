#include <cstring>

#include "harness.hpp"
#include "render.hpp"

TEST(ansi_256_color_emits_expected_sequence) {
  CHECK(std::strcmp(fresco::ansi::fg256(196), "\033[38;5;196m") == 0);
  CHECK(std::strcmp(fresco::ansi::bg256(17), "\033[48;5;17m") == 0);
}

TEST(ansi_truecolor_emits_expected_sequence) {
  CHECK(std::strcmp(fresco::ansi::fgRGB(255, 128, 0), "\033[38;2;255;128;0m") == 0);
  CHECK(std::strcmp(fresco::ansi::bgRGB(0, 0, 0), "\033[48;2;0;0;0m") == 0);
}

TEST(ansi_hex_equals_rgb_builder) {
  CHECK(std::strcmp(fresco::ansi::fgHex(0xFF8000), fresco::ansi::fgRGB(255, 128, 0)) == 0);
  CHECK(std::strcmp(fresco::ansi::bgHex(0x00FF00), fresco::ansi::bgRGB(0, 255, 0)) == 0);
}
