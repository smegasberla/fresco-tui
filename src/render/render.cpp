# include "../include/os_check.hpp"
# include "../include/render.hpp"
# include <iostream>

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
    struct termios raw;

    // Get the ccurrent terminal attributes
    // STDIN_FILENO = Standart for the input data stream (keyboard only)
    tcgetattr(STDIN_FILENO, &raw);

    // Apply local flags
    // ICANON = Disable canon mode && ECHO = Disable printing
    raw.c_lflag &= ~(ICANON | ECHO);

    // Apply new aattributes immedeately
    // TCSAFLUSH = Tells the OS how to handle the apply of the new attributes
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    raw_on = true;
#elif OS_CHECK_WINDOWS

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
        struct termios orig;
        tcgetattr(STDIN_FILENO, &orig);
        orig.c_lflag |= (ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
    #elif OS_CHECK_WINDOWS
        // Put back the console modes we saved in enableRawMode()
        SetConsoleMode(g_in, g_in_mode);
        SetConsoleMode(g_out, g_out_mode);
    #endif
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
    } else {
        perror("ioctl failed");
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
    } else {
        // Keep the 24x80 fallback set at the top of the function
        // GetLastError() is the Windows equivalent of errno
        printf("GetConsoleScreenBufferInfo failed. Error code: %lu\n", GetLastError());
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