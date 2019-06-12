#ifndef CEL_OPENGL_GL_VIEWER_H
#define CEL_OPENGL_GL_VIEWER_H

#if OPENGL_ENABLED

#include <cstddef>

#include <string>
#include <vector>
#include <map>
#include <functional> // for std::function

#include <GL/glew.h>    // must include before glfw
#include <GLFW/glfw3.h>

#include "gl_renderer.h"

#include "../scene.h"
#include "../camera.h"
#include "../viewer.h" // the base class

/*
 * Viewer.h
 * Viewer class mange a window and setting interactive input by user using
 * pixels library. It also configure a renderer and provide it with necessary
 * infomation to render the window.
 *
 * TODO: add a key-response mapping input for construction viewer.
 * TODO: let CEL to have a HUD instance, to render on-screen text
 * TODO: make this thread-safe, since you support multiple window !
 */

namespace CEL {

class GLViewer : public Viewer {

public:

  // Default counstructor : not allowed
  GLViewer() = delete;

  // Initialize with window size and title
  GLViewer(std::size_t window_w, std::size_t window_h, std::string title);

  // Destructor
  ~GLViewer();

  // Start the update&render loop
  void run() override;

private:

  // GL interface object
  GLFWwindow* window;

  // Implementation helpers
  void init_glfw_context(int width, int height, const char* s);
  void init_gl_interface();
  void register_callbacks();
  double last_i, last_j; // used for mouse callback

  // Shared between instances
  using BufferFunc = std::function<void (int w, int h)>;
  using ScrollFunc = std::function<void (double dx, double dy)>;
  using CursorFunc = std::function<void (double i, double j)>;
  using MouseFunc = std::function<void (int button, int action, int modkey)>;
  struct CallbackMemo {
    BufferFunc buffer_callback; // a unique buffer size callback
    ScrollFunc scroll_callback; // a unique scroll callback
    CursorFunc cursor_callback; // a unique cursor position callback
    MouseFunc mouse_callback; // a unique mouse button callback
  };
  static std::map<GLFWwindow*, CallbackMemo> memo_table;
  static void glfw_init_auto();
  static void glfw_error_callback(int, const char*);
  static void glfw_bffersize_callback(GLFWwindow*, int, int);
  static void glfw_scroll_callback(GLFWwindow*, double, double);
  static void glfw_cursor_callback(GLFWwindow*, double, double);
  static void glfw_mouse_callback(GLFWwindow*, int, int, int);
};

} // namespace CEL

#endif // OPENGL_ENABLED

#endif

