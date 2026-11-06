#include "../include/components.hpp"
#include "../include/render.hpp"

void fresco::component::Button::draw(render::Screen& screen) const {

    // Geometry is stored as float but the buffer is in cells, so a component
    // placed at a fractional position lands on the cell it starts in
    const int row = static_cast<int>(y);
    const int col = static_cast<int>(x);

    // A button is a box of `width` cells, so the label never spills out of it.
    // No width means no box was given, and then only the screen clips
    const int cells = static_cast<int>(width);

    if (cells <= 0) {
        screen.write(row, col, text, render::Style{});
        return;
    }

    // shortcut: the cut is in bytes while the box is in cells, so a label with
    // multi-byte characters loses a few cells before the width runs out. write()
    // stops at the broken sequence instead of drawing something wrong. Upgrade
    // when labels become user-supplied
    screen.write(row, col, std::string_view(text).substr(0, static_cast<std::size_t>(cells)),
                 render::Style{});

}
