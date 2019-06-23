#ifndef rei_VIEWER_H
#define rei_VIEWER_H

#include <cstddef>
#include <string>
#include <vector>
#include <memory>

#include "../common.h"
#include "../console.h"
#include "../input.h"
#include "../camera.h"
#include "../scene.h"
#include "../renderer.h"

/**
 * viewer.h
 * Define the interface for viewer, which manage a window and handle user-
 * inputs.
 *
 * Implementation is platform-specific. (see opengl/viewer.h and d3d/viewer.h)
 */

namespace rei {

class Viewer {
public:
  Viewer(std::size_t window_w, std::size_t window_h, std::wstring title) 
    : width(window_w), height(window_h), title(title) {}
  virtual ~Viewer() {};

  virtual void init_viewport(Renderer& renderer) = 0;
  ViewportHandle get_viewport() const { return viewport; }

  virtual void update_title(const std::wstring& title) = 0;
  void set_input_bus(std::weak_ptr<InputBus> input_bus) { this->input_bus = input_bus; }

  [[deprecated]]
  void set_renderer(std::shared_ptr<Renderer> renderer) { DEPRECATED }
  void set_scene(std::shared_ptr<Scene> scene) { DEPRECATED }
  void set_camera(std::shared_ptr<Camera> cam) { DEPRECATED }

  [[deprecated]]
  virtual void run() { DEPRECATED }

protected:
  std::size_t width, height;
  std::wstring title = L"No Title";

  std::weak_ptr<InputBus> input_bus;

  ViewportHandle viewport;
};

[[deprecated]]
std::shared_ptr<Viewer> makeViewer(size_t window_w, size_t window_h, std::string title);

}

#endif
