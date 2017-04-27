#ifndef CEL_RENDERER_H
#define CEL_RENDERER_H

#include <cstddef>

#include "camera.h"
#include "scene.h"
#include "model.h"

/*
 * renderer.h
 * Define the Renderer interface and a soft renderer.
 *
 * TODO: implement fantastic CEL shading
 * TODO: implement a hard renderer
 */

namespace CEL {

// Renderer ///////////////////////////////////////////////////////////////////
// The base class
////

typedef std::vector<unsigned char>::size_type BufferSize;

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

  const Scene* scene = nullptr;
  const Camera* camera = nullptr;

};


// SoftRenderer ///////////////////////////////////////////////////////////////
// The software renderering; everything is done on CPU and main memory.
////

class SoftRenderer : public Renderer {

  typedef unsigned char* BufferPtr;
  typedef std::vector<unsigned char> Buffer;
  typedef void draw_func(BufferPtr, BufferSize, BufferSize);

public:

  // Default constructor
  SoftRenderer();

  // Destructor
  ~SoftRenderer() override {};

  // Configuration
  void set_draw_func(draw_func fp);

  // Render request
  void set_buffer_size(BufferSize width, BufferSize height) override;
  void render() override;

private:

  Buffer buffer;
  BufferSize width, height;
  draw_func f;

  // Implementation helpers
  void rasterize_triangle(BufferPtr b, BufferSize w, BufferSize h);

};


// HardRenderer ///////////////////////////////////////////////////////////////
// This render will need access to the OpenGL API
////
class HardRenerer : public Renderer {

};

} // namespace CEL

#endif
