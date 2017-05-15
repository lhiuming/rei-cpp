#ifndef CEL_RENDERER_H
#define CEL_RENDERER_H

#include <cstddef>

#include "camera.h"
#include "scene.h"
#include "model.h"

/*
 * renderer.h
 * Define the Renderer interface. Implementation is platform-specific.
 * (see opengl/renderer.h and d3d/renderer.h)
 */

namespace CEL {

// Renderer ///////////////////////////////////////////////////////////////////
// The base class
////

using BufferSize = std::size_t;

class Renderer {
public:

  // Default constructor
  Renderer();

  // Destructor
  virtual ~Renderer() {};

  // Change content
  void set_scene(const Scene* scene) { this->scene = scene; }
  void set_camera(const Camera* camera) { this->camera = camera; }

  // Basic interface
  virtual void set_buffer_size(BufferSize width, BufferSize height) = 0;
  virtual void render() = 0;

protected:

  BufferSize width, height;

  const Scene* scene = nullptr;
  const Camera* camera = nullptr;

};

} // namespace CEL

#endif
