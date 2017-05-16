// Source of viewer.h
#include "viewer.h"

#include <string>
#include <memory>

// #ifdef __USE_OPENGL__
#include "opengl/gl_viewer.h"
// #else
// #endif

using namespace std;

namespace CEL {

// A cross-platform viewer factory
shared_ptr<Viewer> makeViewer(size_t window_w, size_t window_h, string title)
{
  // #ifdefine __USE_OPENGL__
  return make_shared<GLViewer>(window_w, window_h, title);
  // #else
  // #endif
}

} // namespace CEL
