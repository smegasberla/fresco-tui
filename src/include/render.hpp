#pragma once
#include <iostream>
#include <cstdint>
#include <vector>

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
constexpr auto CLEAR_SCREEN = "\033[2J";

// Move cursor:  "\033[<row>;<col>H"  — use your own formatting,
// or add a helper:  inline char* cursorMove(int row, int col) { … }

}  // namespace fresco::ansi

namespace fresco::render {

  // Text attributes, one bit each, numbered in SGR order so flush() can walk
  // the bits and emit the matching code without a lookup table.
  // The value in the comment is the SGR parameter each bit corresponds to.
   enum Attr : uint16_t {
    Bold = 1u << 0,            // 1  = increased intensity
    Dim = 1u << 1,             // 2  = decreased intensity (faint)
    Italic = 1u << 2,          // 3  = italic
    Underline = 1u << 3,       // 4  = underline
    Blink = 1u << 4,           // 5  = slow blink
    Reverse = 1u << 5,         // 7  = swap foreground and background (inverse)
    Hidden = 1u << 6,          // 8  = conceal the text
    Strikethrough = 1u << 7,   // 9  = crossed out
    // Not modelled yet, add when something can emit them:
    //   RapidBlink = 6, DoubleUnderline = 21, Framed = 51, Encircled = 52,
    //   Overline = 53, Superscript = 73, Subscript = 74
  };

  // So attrs = Bold | Underline; stays a uint16_t instead of an int
  inline constexpr uint16_t operator|(Attr a, Attr b) {
    return static_cast<uint16_t>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
  }

  inline constexpr uint32_t DEFAULT_COLOR = 0xFFFFFFFF;

  struct Style {

    // flush() compares the current pen with the next cell's style to decide if
    // any SGR code has to be emitted. The defaulted comparison does that field
    // by field, and all three fields are plain numbers, so it is free.
    bool operator==(const Style&) const = default;
    uint32_t fg = DEFAULT_COLOR;
    uint32_t bg = DEFAULT_COLOR;
    uint16_t attrs = 0;

  };

  struct Cell {

    // Two cells are equal when they show the same character with the same style
    // flush() compares the cell it already drew with the new one to know if
    // that position has to be repainted at all
    bool operator==(const Cell&) const = default;
    // Code point
    char32_t cp = U' ';
    // Style
    Style s;

  };

  class Screen {
    
    public:
      // Enable raw mode for the terminal (cross-os)
      void enableRawMode();
      // Disable raw mode for the terminal (cross-os)
      void disableRawMode();
      // Tells the size of the terminal (cross-platform).
      void size(int& row, int& col) const;
      // alt screen + hide cursor + raw mode
      void enter();
      // reverse of enter in reversed order
      void leave();
      // Clears the screen
      void clearScreen();
      // Resizes the back buffer to rows x cols cells and blanks every cell
      // Writes nothing: the terminal is untouched, so it works before enter()
      // and in tests without a pty
      void resize(int rows, int cols);
      // Read-only access to the cell at (row, col)
      // Out of range it returns a shared blank cell instead of crashing, so a
      // caller can read a neighbour while drawing at the edge without checking
      // put() stays the only writer, this never hands out a writable cell
      const Cell& at(int row, int col) const;
      void put(int row, int col, char32_t ch, Style s);

    private:
      // True while enter() owns the terminal, so leave() never undoes a state
      // this object never set (and a second Screen does not undo the first)
      bool entered = false;

      // True while raw mode is on, so disableRawMode() only restores the modes
      // that that this object saved
      bool raw_on = false;

      // Size of the back buffer in cells (not pixels, not characters).
      // resize() is the only place that changes them, so they always match
      // cells.size() == rows * cols
      int rows = 0;
      int cols = 0;

      // One flat row-major array of cells: index = row * cols + col
      // Contiguous on purpose, flush() and clear() walk it linearly, and one
      // allocation per resize is enough. Empty until resize() runs
      std::vector<Cell> cells;

      // Offset of (row, col) inside cells. No bounds check on purpose: the
      // callers (put, at) clip first, so this stays one multiplication
      std::size_t index(int row, int col) const;

      // True when (row, col) is inside the buffer. put() and at() both ask this
      // one function, so writing and reading can never disagree about the edges
      bool inside(int row, int col) const;

      // Sets rows/cols and refills the whole array with blanks (Cell{})
      // Only resize() calls it, so this is the single place that allocates
      void assign(int new_rows, int new_cols);
  };

}  // namespace fresco::render
