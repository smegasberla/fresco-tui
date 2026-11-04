#pragma once
#include <iostream>
#include <string>

#include "common.hpp"

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
            virtual void draw() const = 0;

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
        Button(const std::string& text, float x, float y, float h, float w)
            : Component(x, y, height, width), text(text) {}
    };

}  // namespace fresco::component
