// Source of renderer.h
#include "renderer.h"

#include <iostream>
#include <memory>

// #ifdef __USE_OPENGL__
#include "opengl/gl_renderer.h"
// #else
// #endif

using namespace std;

namespace CEL {

// Default constructor
Renderer::Renderer()
{
  cout << "A empty base renderer is created. " << endl;
}

// A cross-platform renderer factory
shared_ptr<Renderer> makeRenderer()
{
  // #ifdefine __USE_OPENGL__
  return make_shared<GLRenderer>();
  // #else
  // #endif
}

} // namespace CEL
