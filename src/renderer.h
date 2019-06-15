#ifndef REI_RENDERER_H
#define REI_RENDERER_H

#include <cstddef>

#include "camera.h"
#include "model.h"
#include "scene.h"

/*
 * renderer.h
 * Define the Renderer interface. Implementation is platform-specific.
 * (see opengl/renderer.h and d3d/renderer.h)
 */

namespace rei {

// Renderer ///////////////////////////////////////////////////////////////////
// The base class
////

class Renderer {
public:
  // Type Alias
  using BufferSize = std::size_t;

  // Default constructor
  Renderer();

  // Destructor
  virtual ~Renderer() {};

  // Change content
  virtual void set_scene(std::shared_ptr<const Scene> scene) { this->scene = scene; }
  virtual void set_camera(std::shared_ptr<const Camera> camera) { this->camera = camera; }

  // Basic interface
  virtual void set_buffer_size(BufferSize width, BufferSize height) = 0;
  virtual void render() = 0;

protected:
  BufferSize width, height;

  std::shared_ptr<const Scene> scene;
  std::shared_ptr<const Camera> camera;
};

// A cross-platform renderer factory
std::shared_ptr<Renderer> makeRenderer();

} // namespace REI

#endif
