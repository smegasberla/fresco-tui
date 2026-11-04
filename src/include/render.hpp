#pragma once
#include <iostream>

#include "../include/common.hpp"
#include "../include/components.hpp"

/*
 * Contains the basics of the renderer
 */
namespace fresco::ansi {

// ── Core ──────────────────────────────────────────────────────────────
constexpr auto ESC = "\033";
constexpr auto RESET = "\033[0m";
constexpr auto RESET_FG = "\033[39m";
constexpr auto RESET_BG = "\033[49m";

// ── Text attributes (SGR) ────────────────────────────────────────────
constexpr auto BOLD = "\033[1m";
constexpr auto DIM = "\033[2m";
constexpr auto ITALIC = "\033[3m";
constexpr auto UNDERLINE = "\033[4m";
constexpr auto BLINK = "\033[5m";
constexpr auto REVERSE = "\033[7m";
constexpr auto HIDDEN = "\033[8m";
constexpr auto STRIKETHROUGH = "\033[9m";

// ── Standard foreground  (30–37) ─────────────────────────────────────
constexpr auto FG_BLACK = "\033[30m";
constexpr auto FG_RED = "\033[31m";
constexpr auto FG_GREEN = "\033[32m";
constexpr auto FG_YELLOW = "\033[33m";
constexpr auto FG_BLUE = "\033[34m";
constexpr auto FG_MAGENTA = "\033[35m";
constexpr auto FG_CYAN = "\033[36m";
constexpr auto FG_WHITE = "\033[37m";

// ── Bright foreground  (90–97) ───────────────────────────────────────
constexpr auto FG_BRIGHT_BLACK = "\033[90m";
constexpr auto FG_BRIGHT_RED = "\033[91m";
constexpr auto FG_BRIGHT_GREEN = "\033[92m";
constexpr auto FG_BRIGHT_YELLOW = "\033[93m";
constexpr auto FG_BRIGHT_BLUE = "\033[94m";
constexpr auto FG_BRIGHT_MAGENTA = "\033[95m";
constexpr auto FG_BRIGHT_CYAN = "\033[96m";
constexpr auto FG_BRIGHT_WHITE = "\033[97m";

// ── Standard background  (40–47) ─────────────────────────────────────
constexpr auto BG_BLACK = "\033[40m";
constexpr auto BG_RED = "\033[41m";
constexpr auto BG_GREEN = "\033[42m";
constexpr auto BG_YELLOW = "\033[43m";
constexpr auto BG_BLUE = "\033[44m";
constexpr auto BG_MAGENTA = "\033[45m";
constexpr auto BG_CYAN = "\033[46m";
constexpr auto BG_WHITE = "\033[47m";

// ── Bright background  (100–107) ─────────────────────────────────────
constexpr auto BG_BRIGHT_BLACK = "\033[100m";
constexpr auto BG_BRIGHT_RED = "\033[101m";
constexpr auto BG_BRIGHT_GREEN = "\033[102m";
constexpr auto BG_BRIGHT_YELLOW = "\033[103m";
constexpr auto BG_BRIGHT_BLUE = "\033[104m";
constexpr auto BG_BRIGHT_MAGENTA = "\033[105m";
constexpr auto BG_BRIGHT_CYAN = "\033[106m";
constexpr auto BG_BRIGHT_WHITE = "\033[107m";

// ── 256-color (38;5;N / 48;5;N) ──────────────────────────────────────
// Build the full sequence:  fresco::ansi::fg256(196)  →  "\033[38;5;196m"
inline const char* fg256(int n) {
  static thread_local char buf[16];
  std::snprintf(buf, sizeof buf, "\033[38;5;%dm", n);
  return buf;
}
inline const char* bg256(int n) {
  static thread_local char buf[16];
  std::snprintf(buf, sizeof buf, "\033[48;5;%dm", n);
  return buf;
}

// ── True color / 24-bit (38;2;R;G;B / 48;2;R;G;B) ───────────────────
// Build the full sequence:  fresco::ansi::fgRGB(255, 128, 0)  →  "\033[38;2;255;128;0m"
inline const char* fgRGB(int r, int g, int b) {
  static thread_local char buf[24];
  std::snprintf(buf, sizeof buf, "\033[38;2;%d;%d;%dm", r, g, b);
  return buf;
}
inline const char* bgRGB(int r, int g, int b) {
  static thread_local char buf[24];
  std::snprintf(buf, sizeof buf, "\033[48;2;%d;%d;%dm", r, g, b);
  return buf;
}

// ── Hex convenience (accepts 0xRRGGBB) ───────────────────────────────
inline const char* fgHex(unsigned int hex) {
  return fgRGB((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF);
}
inline const char* bgHex(unsigned int hex) {
  return bgRGB((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF);
}

// ── Cursor & screen-buffer control ───────────────────────────────────
constexpr auto CURSOR_HOME = "\033[H";
constexpr auto CURSOR_SAVE = "\033[s";
constexpr auto CURSOR_RESTORE = "\033[u";
constexpr auto HIDE_CURSOR = "\033[?25l";
constexpr auto SHOW_CURSOR = "\033[?25h";

constexpr auto ENTER_ALT_SCREEN = "\033[?1049h";
constexpr auto LEAVE_ALT_SCREEN = "\033[?1049l";

// Move cursor:  "\033[<row>;<col>H"  — use your own formatting,
// or add a helper:  inline char* cursorMove(int row, int col) { … }

}  // namespace fresco::ansi

namespace fresco::render {

class Screen {
  
  public:
    // Enable raw mode for the terminal (cross-os)
    void enableRawMode();
    // Disable raw mode for the terminal (cross-os)
    void disableRawMode();
    // Tells the size of the terminal.
    void size(int& row, int& col) const;
  
};

}  // namespace fresco::render
