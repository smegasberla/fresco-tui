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

class Button : public fresco::Component {
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
