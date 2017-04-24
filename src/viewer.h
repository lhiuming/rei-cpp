#ifndef CEL_VIEWER_H
#define CEL_VIEWER_H

#include <string>

#include "pixels.h"
#include "scene.h"
#include "renderer.h"

/*
 * Viewer.h
 * Viewer class mange a window and setting interactive input by user using
 * pixels library;
 *
 * TODO: add a key-response mapping input for construction viewer.
 * TODO: let CEL to have a HUD instance, to render on-screen text
 * TODO: make this thread-safe, since you support multiple window !
 */

namespace CEL {

class Viewer {

public:

  // Constructors
  Viewer() = delete;
  Viewer(size_t window_w, size_t window_h, std::string title) {
    gl_init();
    this->window = gl_create_window(window_w, window_h, title.c_str());
    gl_get_buffer_size(this->window, this->buffer_w, this->buffer_h);
    ++view_count; // increase the internal count
  }

  // Destructor
  ~Viewer() {
    gl_destroy_window(window);
    --view_count;
    if (view_count == 0) gl_terminate();
  }

  // operations
  void set_renderer(Renderer* renderer) {
    this->renderer = renderer;
  }
  void set_scene(Scene* scene) {
    this->scene = scene;
  }

private:

  WindowID window;
  size_t buffer_w, buffer_h;

  Renderer* renderer; // a pointer to a renderer
  Scene* scene; // a pointer to a scene

  static int view_count; // count the number of viewer window

};

// Static variable initialization
int Viewer::view_count = 0;

} // namespace CEL


#endif
