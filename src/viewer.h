#ifndef CEL_VIEWER_H
#define CEL_VIEWER_H

#include <cstddef>
#include <string>
#include <vector>

#include "renderer.h"
#include "scene.h"
#include "camera.h"

/**
 * viewer.h
 * Define the interface for viewer, which manage a window and handle user-
 * inputs.
 *
 * Implementation is platform-specific. (see opengl/viewer.h and d3d/viewer.h)
 */

namespace CEL {

class Viewer {

public:

  // Default counstructor
  Viewer() {};

  // Initialize with window size and title
  Viewer(std::size_t window_w, std::size_t window_h, std::string title);

  // Destructor
  virtual ~Viewer();

  // Configuration
  virtual void set_renderer(Renderer* renderer);
  virtual void set_scene(Scene* scene);
  virtual void set_camera(Camera* cam);

  // Start the update&render loop
  virtual void run();

};

} // namespace CEL


#endif
