#ifndef CEL_VIEWER_H
#define CEL_VIEWER_H

#include <cstddef>
#include <string>
#include <vector>

#include "camera.h"
#include "renderer.h"
#include "scene.h"

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
  virtual ~Viewer() {};

  // Configuration
  void set_renderer(std::shared_ptr<Renderer> renderer) { this->renderer = renderer; }
  void set_scene(std::shared_ptr<Scene> scene) { this->scene = scene; }
  void set_camera(std::shared_ptr<Camera> cam) { this->camera = cam; }

  // Start the update&render loop
  virtual void run() = 0;

protected:
  std::shared_ptr<Renderer> renderer; // a pointer to a renderer
  std::shared_ptr<Scene> scene;       // a pointer to a scene
  std::shared_ptr<Camera> camera;     // a pointer to a camera
};

// A cross-platform viewer factory
std::shared_ptr<Viewer> makeViewer(size_t window_w, size_t window_h, std::string title);

} // namespace CEL

#endif
