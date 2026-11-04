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

static bool g_raw = false;

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
    g_raw = true;
#elif OS_CHECK_WINDOWS
    g_in  = GetStdHandle(STD_INPUT_HANDLE);
    g_out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleMode(g_in, &g_in_mode))  return;      /* not a console */
    if (!GetConsoleMode(g_out, &g_out_mode)) return;

    SetConsoleMode(g_in,
        (g_in_mode & ~(DWORD)(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT |
                                ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT |
                                ENABLE_QUICK_EDIT_MODE))
        | ENABLE_EXTENDED_FLAGS);

    SetConsoleMode(g_out,
        (g_out_mode & ~(DWORD)ENABLE_PROCESSED_OUTPUT)
        | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    g_raw = true;
#endif
}

void render::Screen::disableRawMode() {
    if (!g_raw) return;
    #if OS_CHECK_UNIX
        struct termios orig;
        tcgetattr(STDIN_FILENO, &orig);
        orig.c_lflag |= (ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
    #elif OS_CHECK_WINDOWS
        SetConsoleMode(g_in, g_in_mode);
        SetConsoleMode(g_out, g_out_mode);
    #endif
    g_raw = false;
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

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        col = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        row = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    } else {
        printf("GetConsoleScreenBufferInfo failed. Error code: %lu\n", GetLastError());
    }

#endif

}