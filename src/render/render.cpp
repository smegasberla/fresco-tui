# include "../include/os_check.hpp"
# include "../include/render.hpp"
# include <iostream>
# include <string>

#if OS_CHECK_UNIX
#  include <termios.h>
#  include <unistd.h>
#  include <sys/ioctl.h>
#elif OS_CHECK_WINDOWS
#  include <windows.h>
#endif

#if OS_CHECK_WINDOWS
static HANDLE g_in, g_out;
static DWORD  g_in_mode, g_out_mode;
#endif

namespace render = fresco::render;

void render::Screen::enableRawMode() {
#if OS_CHECK_UNIX

    // Already raw: keep the first snapshot. Saving a second time would store the
    // raw attributes and then "restore" those
    if (raw_on)
        return;

    // tcgetattr fails when stdin is not a terminal (a pipe, a file, under make),
    // and then there is nothing to save: raw mode simply stays off
    if (tcgetattr(STDIN_FILENO, &saved_termios) != 0)
        return;

    struct termios raw = saved_termios;

    // Apply local flags
    // ICANON = Disable canon mode && ECHO = Disable printing
    raw.c_lflag &= ~(ICANON | ECHO);

    // TCSAFLUSH = Tells the OS how to handle the apply of the new attributes
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0)
        return;

    // Only now is there something to undo, so only now is the flag raised
    raw_on = true;
#elif OS_CHECK_WINDOWS

    // Same rule as POSIX: a second enable must not overwrite the saved modes
    if (raw_on)
        return;

    // Get the handles of the console: input is the keyboard, output is the screen
    g_in  = GetStdHandle(STD_INPUT_HANDLE);
    g_out = GetStdHandle(STD_OUTPUT_HANDLE);

    // GetConsoleMode fails when the handle is not a console (a pipe or a file)
    // We keep the current modes in g_in_mode / g_out_mode so disableRawMode()
    // can put them back exactly as they were
    if (!GetConsoleMode(g_in, &g_in_mode))  return;      /* not a console */
    if (!GetConsoleMode(g_out, &g_out_mode)) return;

    // Clear the input flags:
    // ENABLE_LINE_INPUT = same as ICANON, every key arrives immediately
    // ENABLE_ECHO_INPUT = same as ECHO, the typed keys are not printed
    // ENABLE_PROCESSED_INPUT = Ctrl-C arrives as a byte instead of killing us
    // ENABLE_MOUSE_INPUT = we only want keys, not mouse events
    // ENABLE_QUICK_EDIT_MODE = a mouse click would freeze the console, so no selection
    // ENABLE_EXTENDED_FLAGS = needed, otherwise we cannot change QUICK_EDIT_MODE
    SetConsoleMode(g_in,
        (g_in_mode & ~(DWORD)(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT |
                                ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT |
                                ENABLE_QUICK_EDIT_MODE))
        | ENABLE_EXTENDED_FLAGS);

    // Clear ENABLE_PROCESSED_OUTPUT: "\n" is no longer turned into "\r\n"
    // Set ENABLE_VIRTUAL_TERMINAL_PROCESSING: Windows understands the ANSI
    // escape codes (colors, cursor moves, alt screen) without them being printed
    SetConsoleMode(g_out,
        (g_out_mode & ~(DWORD)ENABLE_PROCESSED_OUTPUT)
        | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    raw_on = true;
#endif
}

void render::Screen::disableRawMode() {
    if (!raw_on) return;
    #if OS_CHECK_UNIX
        // Write back exactly what enableRawMode() found. Reading the attributes
        // now and turning ICANON and ECHO on would keep whatever else changed in
        // the meantime, and would undo only those two flags
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_termios);
    #elif OS_CHECK_WINDOWS
        // Put back the console modes we saved in enableRawMode()
        SetConsoleMode(g_in, g_in_mode);
        SetConsoleMode(g_out, g_out_mode);
    #endif
    // A failed restore is not retried: the terminal is being handed back, and a
    // second attempt would only repeat the same error
    raw_on = false;
}

void render::Screen::size(int& row, int& col) const {
    row = 24;   // fallback: sane size when stdout is not a terminal
    col = 80;   // fallback: sane size when stdout is not a terminal

#if OS_CHECK_UNIX

    struct winsize ws;

    // TIOCGWINSZ: Terminal Input/Output Control Get Window Size
    // Here we request the size of the terminal in 
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        row = ws.ws_row;
        col = ws.ws_col;
    }


#elif OS_CHECK_WINDOWS

    // Get the handle of the console we are writing to
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // CONSOLE_SCREEN_BUFFER_INFO holds the whole buffer (dwSize) and the part
    // the user can actually see (srWindow). We take the window, because the
    // buffer is usually much taller (the scrollback)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        // +1 because Right and Bottom are inclusive coordinates
        col = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        row = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
    
#endif

}

void render::Screen::enter() {


#if OS_CHECK_UNIX

    // Check if the output is a terminal
    if (isatty(STDOUT_FILENO)) {

        // If entered is already true, we do nothing
        if (entered)
            return; 

        // We enable raw mode    
        render::Screen::enableRawMode();
        int row, col;

        // Get the size of the terminal
        size(row, col);
        resize(row, col);

        // Emit the ansi escape codes for ENTER_ALT_SCREEN + CURSOR_HOME + RESET
        std::string out;
        out.reserve(row * col * 4 + 256);

        out.append(fresco::ansi::ENTER_ALT_SCREEN);
        out.append(fresco::ansi::CURSOR_HOME);
        out.append(fresco::ansi::RESET);
        out.append(fresco::ansi::HIDE_CURSOR);

        // We write the escape codes to stdout
        std::cout.write(out.data(), out.size()); 

        // We immedeaately flush stdout
        std::cout.flush();
        // Set entered to true
        entered = true;
    } else {
        // Not a terminal: nothing was entered.
        entered = false;
    }

#elif OS_CHECK_WINDOWS

    // Windows equivalent of isatty(): GetConsoleMode fails when the output
    // is redirected to a file or to a pipe
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD out_mode = 0;
    if (GetConsoleMode(hOut, &out_mode)) {

        // If entered is already true, we do nothing
        if (entered)
            return;

        // We enable raw mode
        // This also turns on VIRTUAL_TERMINAL_PROCESSING, which is what makes
        // Windows understand the ANSI escape codes we write below
        render::Screen::enableRawMode();

        int row, col;

        // Get the size of the terminal
        size(row, col);
        resize(row, col);

        // Emit the ansi escape codes for ENTER_ALT_SCREEN + CURSOR_HOME + RESET
        std::string out;
        out.reserve(row * col * 4 + 256);

        out.append(fresco::ansi::ENTER_ALT_SCREEN);
        out.append(fresco::ansi::CURSOR_HOME);
        out.append(fresco::ansi::RESET);
        out.append(fresco::ansi::HIDE_CURSOR);

        // We write the escape codes to stdout
        std::cout.write(out.data(), out.size());

        // We immedeately flush stdout
        std::cout.flush();

        // Set entered to true
        entered = true;
    } else {
        // Not a console: nothing was entered.
        entered = false;
    }

#endif

}

void render::Screen::leave() {

    // If we didn't enter, we do nothing
    if(!entered)
        return;

    // Declare variables for columns and rows    
    std::string out;

    // Append the ansi codes
    out.append(fresco::ansi::RESET);
    out.append(fresco::ansi::SHOW_CURSOR);
    out.append(fresco::ansi::LEAVE_ALT_SCREEN);

    // We write the escape codes to stdout
    std::cout.write(out.data(), out.size());

    // We immedeately flush stdout
    std::cout.flush();

    // We disable raw mode
    disableRawMode();

    // Finally set entered to false
    entered = false;


}

void render::Screen::clearScreen() {

     // If we didn't enter, we do nothing
    if (!entered)
        return;

    // Declare variables for columns and rows    
    std::string out;

    // Append the ansi codes
    out.append(ansi::RESET);
    out.append(ansi::CLEAR_SCREEN);
    out.append(ansi::CURSOR_HOME);

    // We write the escape codes to stdout
    std::cout.write(out.data(), out.size());

    // We immedeately flush stdout
    std::cout.flush();

}

std::size_t render::Screen::index(int row, int col) const {

    // The buffer is one flat array, so a 2D position is just an offset.
    // Cast to size_t before multiplying: as int, a big terminal would overflow
    return static_cast<std::size_t>(row) * static_cast<std::size_t>(cols) +
           static_cast<std::size_t>(col);

}

void render::Screen::assign(int new_rows, int new_cols) {

    // Remember the new size first, then rebuild the array in one call.
    // assign() overwrites the cells that already existed too, so growing after
    // a shrink never brings old content back
    rows = new_rows;
    cols = new_cols;

    const std::size_t count = static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols);

    // Cell{} is a blank cell (space + default style), so no fill loop is needed
    cells.assign(count, Cell{});

}

void render::Screen::resize(int rows, int cols) {

    // Negative sizes mean nothing here: treat them as empty instead of letting
    // them turn into a huge size_t and an enormous allocation
    if (rows < 0)
        rows = 0;
    if (cols < 0)
        cols = 0;

    // Same size as now: do nothing, so calling it every frame costs no allocation
    if (rows == this->rows && cols == this->cols)
        return;

    // Different size: rebuild the buffer (this is the only allocation)
    assign(rows, cols);

}

// The one place that decides where the buffer ends. put() and at() both come
// through here, so a write and a read can never disagree about the edges
bool render::Screen::inside(int row, int col) const {

    // Strictly >= 0 and < size: the last valid row is rows - 1, not rows
    return row >= 0 && row < rows && col >= 0 && col < cols;

}

// Read-only access to one cell. The bounds check has to come before index():
// index() does not check anything, and a negative row would become a huge
// size_t offset, which is undefined behaviour instead of a blank cell
const render::Cell& render::Screen::at(int row, int col) const {

    // Outside the buffer: hand back a shared blank cell, so reading a neighbour
    // while drawing at the edge never crashes and the caller needs no check
    if (!inside(row, col)) {
        // Built once for the whole program. It is const, so nobody can write
        // through the reference we hand out
        static const Cell blank{};   
        return blank;
    }
    
    // Inside the buffer: just the flat offset, no copy is made
    return cells[index(row, col)];

}

void render::Screen::put(int row, int col, char32_t ch, render::Style s) {

    // Outside the buffer: drop it silently, the same rule at() follows for reads
    if (!inside(row, col))
        return;

    // Store one cell: the character plus its style. Nothing is sent to the
    // terminal here, that is flush()'s job
    cells[index(row, col)] = Cell{ch, s};

} 

void render::Screen::write(int row, int col, std::string_view text, Style s) {

    // text is bytes, Cell::cp is a code point: walk the UTF-8 ourselves and
    // store one cell per code point. Nothing is sent to the terminal here
    std::size_t i = 0;

    while (i < text.size()) {

        const auto lead = static_cast<unsigned char>(text[i]);

        // Length and the bits already decoded come from the lead byte:
        // 0xxxxxxx 1 byte, 110xxxxx 2, 1110xxxx 3, 11110xxx 4
        std::size_t len;
        char32_t cp;

        if (lead < 0x80) {
            len = 1;
            cp = lead;
        } else if ((lead & 0xE0) == 0xC0) {
            len = 2;
            cp = lead & 0x1F;
        } else if ((lead & 0xF0) == 0xE0) {
            len = 3;
            cp = lead & 0x0F;
        } else if ((lead & 0xF8) == 0xF0) {
            len = 4;
            cp = lead & 0x07;
        } else {
            // Neither ASCII nor a lead byte: the string is broken, so stop
            // shortcut: the rest is dropped instead of becoming U+FFFD,
            // upgrade when callers start passing untrusted text
            break;
        }

        // Not enough bytes left for the length the lead byte promised: stop
        // before reading past the end of the view
        if (i + len > text.size())
            break;

        // Every following byte must look like 10xxxxxx and carries 6 bits
        bool valid = true;
        for (std::size_t k = 1; k < len; ++k) {
            const auto next = static_cast<unsigned char>(text[i + k]);

            if ((next & 0xC0) != 0x80) {
                valid = false;
                break;
            }

            cp = (cp << 6) | (next & 0x3F);
        }

        if (!valid)
            break;

        // Control bytes (\n, \t, \033 and friends) are not printable: a cell
        // holding one would move the cursor or emit an escape during flush()
        // and shift the rest of the row. Skip them without taking a column
        if (cp < 0x20 || cp == 0x7F) {
            i += len;
            continue;
        }

        // put() stores the cell and owns the clipping, so an off-screen row or
        // column is dropped silently here too. col counts cells, never bytes
        put(row, col, cp, s);

        ++col;
        i += len;
    }

}

// One cell holds a code point, the terminal wants UTF-8 bytes. This is the
// inverse of the decoder in write(): 1 to 4 bytes, most significant first
static void append_codepoint(std::string& out, char32_t cp) {

    // put() stores any code point it is given, so a control byte can arrive
    // here. Emitting it raw would move the cursor or start an escape, and
    // dropping the cell would shift the rest of the row: show a space instead
    if (cp < 0x20 || cp == 0x7F || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
        cp = U' ';

    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }

}

// Turns one Style into a reset plus the SGR codes it needs. The reset comes
// first because SGR has a code to switch each attribute on but none to switch
// them all off, so the pen is cleared and rebuilt instead of patched
// shortcut: a style change costs a reset plus the full new style, not only the
// difference. Upgrade if profiling says those extra bytes matter
static void append_style(std::string& out, const render::Style& s) {

    // Parameter for each attribute bit. Not bit + 1: SGR has no code 6, it uses
    // 7, 8 and 9 for reverse, hidden and strikethrough
    static constexpr uint16_t param[8] = {1, 2, 3, 4, 5, 7, 8, 9};

    std::string codes;

    for (int bit = 0; bit < 8; ++bit) {
        if (!(s.attrs & (1u << bit)))
            continue;

        if (!codes.empty())
            codes += ';';

        codes += std::to_string(param[bit]);
    }

    // 24-bit color: 38;2;R;G;B for the foreground, 48;2;R;G;B for the back.
    // DEFAULT_COLOR is not a real color, it means "leave the terminal as is"
    if (s.fg != render::DEFAULT_COLOR) {
        if (!codes.empty())
            codes += ';';

        codes += "38;2;" + std::to_string((s.fg >> 16) & 0xFF) + ';' +
                 std::to_string((s.fg >> 8) & 0xFF) + ';' + std::to_string(s.fg & 0xFF);
    }

    if (s.bg != render::DEFAULT_COLOR) {
        if (!codes.empty())
            codes += ';';

        codes += "48;2;" + std::to_string((s.bg >> 16) & 0xFF) + ';' +
                 std::to_string((s.bg >> 8) & 0xFF) + ';' + std::to_string(s.bg & 0xFF);
    }

    // Reset always, then the new style only if it asks for anything
    out += fresco::ansi::RESET;

    if (!codes.empty()) {
        out += "\033[";
        out += codes;
        out += 'm';
    }

}

std::string render::Screen::frame() const {

    std::string out;

    // Nothing to draw: resize() never ran, so there is no frame at all
    if (rows <= 0 || cols <= 0)
        return out;

    // One string for the whole frame, so stdout sees one write instead of one
    // per cell. 4 bytes is the widest UTF-8 sequence, 32 the escapes around it
    out.reserve(static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols) * 4 + 32);

    // The frame starts on the default pen, and RESET is what puts it there.
    // No CURSOR_HOME here: the move that opens row 0 lands on the same cell
    out += ansi::RESET;

    // What the terminal is currently told to use. Starting at Style{} matches
    // the RESET above, so a cell with the default style emits no code at all
    Style pen{};

    for (int row = 0; row < rows; ++row) {

        // Absolute move at the start of each row. Relying on the wrap at the
        // end of the previous row would put the frame off by a cell as soon as
        // a wide character, or a terminal that is not exactly cols wide, shows
        // up on that row
        out += "\033[";
        out += std::to_string(row + 1);
        out += ";1H";

        for (int col = 0; col < cols; ++col) {

            // Bottom-right cell: writing it makes the terminal wrap, and a wrap
            // on the last row scrolls the whole alt screen up by one line,
            // which shifts every frame from then on. One skipped cell because
            // of that, the same trick every full-screen TUI uses
            if (row == rows - 1 && col == cols - 1)
                continue;

            const Cell& cell = cells[index(row, col)];

            // Style is a pen: only a change costs bytes, so a run of cells
            // sharing a style emits one sequence instead of one per cell
            if (!(cell.s == pen)) {
                append_style(out, cell.s);
                pen = cell.s;
            }

            append_codepoint(out, cell.cp);
        }
    }

    // Leave the terminal on the default pen, otherwise whatever prints next
    // (the shell prompt after leave()) inherits the last cell's colors
    out += ansi::RESET;

    return out;

}

void render::Screen::flush() {

    if(!entered)
        return;

    // The frame is built without a terminal, writing it is the only step that
    // needs one. One write, then one stdout flush: the frame lands at once, so
    // no partial row is ever visible while the rest is still being built
    const std::string out = frame();

    if (out.empty())
        return;

    std::cout.write(out.data(), out.size());
    std::cout.flush();

}