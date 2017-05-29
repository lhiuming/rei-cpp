// Source of renderer.h
#include "renderer.h"

#include <iostream>
#include <memory>

#ifdef USE_OPENGL
#include "opengl/gl_renderer.h"
#endif
#ifdef USE_DIRECT3D
#include "direct3d/d3d_renderer.h"
#endif

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
  #ifdef USE_OPENGL
  return make_shared<GLRenderer>();
  #endif
  #ifdef USE_DIRECT3D
  return make_shared<D3DRenderer>();
  #endif
}

} // namespace CEL
