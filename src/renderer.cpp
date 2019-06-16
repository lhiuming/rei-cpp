// Source of renderer.h
#include "renderer.h"

#include <iostream>
#include <memory>

namespace rei {

// Default constructor
Renderer::Renderer() {}

// A cross-platform renderer factory
std::shared_ptr<Renderer> makeRenderer() {
  return nullptr;
}

} // namespace REI
