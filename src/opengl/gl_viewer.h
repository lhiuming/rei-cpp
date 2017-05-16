#ifndef CEL_OPENGL_GL_VIEWER_H
#define CEL_OPENGL_GL_VIEWER_H

#include <cstddef>
#include <string>
#include <vector>

#include "pixels.h"
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

  WindowID window;

  static int view_count; // count the number of alive window

  // Implementation helpers
  void update_buffer_size() const;

  // create callback objects for pixiels library
  BufferFunc make_buffer_callback() const;
  ScrollFunc make_scroll_callback() const;
  CursorFunc make_cursor_callback() const;
  mutable double last_i, last_j;

};

} // namespace CEL


#endif
