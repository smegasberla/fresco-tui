#include <termios.h>
#include <iostream>
#include "../include/render.hpp"
#include <unistd.h>

namespace render = fresco::render;
namespace ansi = fresco::ansi;

void render::Screen::enableRawMode() {

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

}

void render::Screen::disableRawMode() {

    // Here we do the opposite of what we do in enableRawMode()
    struct termios orig;
    tcgetattr(STDIN_FILENO, &orig);
    orig.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}