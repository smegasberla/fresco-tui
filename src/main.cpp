#include <iostream>
#include "include/render.hpp"

namespace render = fresco::render;
namespace ansi = fresco::ansi;

int main() {

    fresco::render::Screen screen;

    screen.enter();

    std::cout << "Raw mode activated, press q to quit..." << std::endl;
    

    char ch;
    while (std::cin.get(ch) && ch != 'q') {
        std::cout << "You pressed: [" << ch << "]" << std::endl;
    }

    screen.leave();
    return 0; 

}