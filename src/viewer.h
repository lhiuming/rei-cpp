#include <string>

#include "pixels.h"

/*
 * Viewer.h
 * Viewer class mange a window and setting interactive input by user using
 * pixels library;
 *
 * TODO: add a Renderer member
 * TODO: add a Scene member
 * TODO: add a key-response mapping input for construction viewer.
 * TODO: let CEL to have a HUD instance, to render on-screen text
 * TODO: make this thread-safe, since you support multiple window !
 */

namespace CEL {

class Viewer {

public:

  // Constructors
  Viewer() = delete;
  Viewer(size_t windw_w, size_t window_h, std::string title) {
    gl_init();
    ++view_count;
    this->window = gl_create_window(window_w, window_h, title.c_str());
    gl_get_buffer_size(this->buffer_w, this->buffer_h);
  }

  // Destructor
  ~View() {
    gl_destroy_window(window);
    --view_count;
    if (view_count == 0) gl_terminate();
  }

private:

  WindowID window;
  size_t buffer_w, buffer_h;

  static view_count = 0; // count the number of viewer window

};

} // namespace CEL
