#pragma once
#include <iostream>
#include <string>

#include "common.hpp"

// draw() only takes a Screen by reference, so a forward declaration is enough
// here and keeps this header from pulling in the whole renderer
namespace fresco::render {
    class Screen;
}

/*
 *
 * Here we define the declaration of the components classes
 * The actual implementation for each lives at src/render/<component>.cpp
 *
 * */
namespace fresco::component {

    class Component {
        // Variables
        protected:
            float x;
            float y;
            float height;
            float width;

        // Functions
        public:
            // This won't allow new Component() to compile
            virtual ~Component() = default;

            // Draws the component into the screen's back buffer. Nothing reaches
            // the terminal until the caller flushes the screen
            virtual void draw(render::Screen& screen) const = 0;

            // Default constructor
            // If we do Button() = default, Button will inherit those defaults
            Component() : x(0), y(0), height(0), width(0) {}

            // constructor — initializes the geometry
            Component(float x, float y, float height, float width)
                : x(x), y(y), height(height), width(width) {}
    };

    class Button : public fresco::component::Component {
        private:
            std::string text = "";

        public:
        // Default constructor
        Button() = default;

        // constructor — initializes the geometry
        // h and w are the size to store: naming the base members here instead
        // would read them before they are set and leave every Button at 0,0
        Button(const std::string& text, float x, float y, float h, float w)
            : Component(x, y, h, w), text(text) {}

        // Writes text at (x, y) in the default style, clipped to the box width.
        // Screen clipping still applies on top of that, so a button hanging over
        // an edge loses the cells that do not fit either
        void draw(render::Screen& screen) const override;
    };

}  // namespace fresco::component
