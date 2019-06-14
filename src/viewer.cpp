// Source of viewer.h
#include "viewer.h"

#include <memory>
#include <string>

#ifdef OPENGL_ENABLED
#include "opengl/gl_viewer.h"
#endif
#ifdef DIRECT3D_ENABLED
#include "direct3d/d3d_viewer.h"
#endif

using namespace std;

namespace CEL {

// A cross-platform viewer factory
shared_ptr<Viewer> makeViewer(size_t window_w, size_t window_h, string title) {
#ifdef OPENGL_ENABLED
  return make_shared<GLViewer>(window_w, window_h, title);
#endif
#ifdef DIRECT3D_ENABLED
  return make_shared<D3DViewer>(window_w, window_h, title);
#endif
}

} // namespace CEL
