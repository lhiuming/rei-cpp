#ifndef CEL_VIEWER_H
#define CEL_VIEWER_H

#include <string>
#include <vector>

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

  Viewer() = delete;
  Viewer(std::size_t window_w, std::size_t window_h, std::string title) ;
  ~Viewer() ;

  // operations
  void set_renderer(Renderer* renderer) ;
  void set_scene(Scene* scene) ;
  void run(); // start the update&render loop

private:

  WindowID window;
  std::size_t buffer_w, buffer_h;    // size of the pixel buffer
  std::vector<unsigned char> pixels; // the pixel buffer

  Renderer* renderer = nullptr; // a pointer to a renderer
  Scene* scene = nullptr; // a pointer to a scene

  static int view_count; // count the number of viewer window

  // private helper
  void update_buffer_size();
  void sleep_alittle();

};

} // namespace CEL


#endif
