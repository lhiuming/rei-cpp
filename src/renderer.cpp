// Source of renderer.h
#include "renderer.h"

#include <iostream>
#include <memory>

#ifdef OPENGL_ENABLED
#include "opengl/gl_renderer.h"
#endif
#ifdef DIRECT3D_ENABLED
#include "direct3d/d3d_renderer.h"
#endif

using namespace std;

namespace CEL {

// Default constructor
Renderer::Renderer() {}

// A cross-platform renderer factory
shared_ptr<Renderer> makeRenderer() {
#ifdef OPENGL_ENABLED
  return make_shared<GLRenderer>();
#endif
#ifdef DIRECT3D_ENABLED
  return make_shared<D3DRenderer>();
#endif
}

} // namespace CEL
