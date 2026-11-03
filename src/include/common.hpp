#pragma once
#include <iostream>

// Basic component class structure
/*
 *
 * This will serve for other components to base on this so that they can have
 * their own draw functions
 *
 * */
namespace fresco {

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

}  // namespace fresco
