// Source of viewer.h
#include "viewer.h"

#include <string>
#include <memory>

#ifdef USE_OPENGL
#include "opengl/gl_viewer.h"
#endif
#ifdef USE_DIRECT3D
#include "direct3d/d3d_viewer.h"
#endif

using namespace std;

namespace CEL {

// A cross-platform viewer factory
shared_ptr<Viewer> makeViewer(size_t window_w, size_t window_h, string title)
{
  #ifdef USE_OPENGL
  return make_shared<GLViewer>(window_w, window_h, title);
  #endif
  #ifdef USE_DIRECT3D
  return make_shared<D3DViewer>(window_w, window_h, title);
  #endif
}

} // namespace CEL
