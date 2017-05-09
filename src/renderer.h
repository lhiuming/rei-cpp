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


// SoftRenderer ///////////////////////////////////////////////////////////////
// The software renderering; everything is done on CPU and main memory.
////

class SoftRenderer : public Renderer {
public:

  // Type Alias
  using Buffer = unsigned char*;
  using DrawFunc = std::function<void (Buffer, BufferSize, BufferSize)>;

  // Default constructor
  SoftRenderer();

  // Destructor
  ~SoftRenderer() override {};

  // Configuration
  void set_draw_func(DrawFunc fp);

  // Render request
  void set_buffer_size(BufferSize width, BufferSize height) override;
  void render() override;

private:

  std::vector<unsigned char> buffer_maker;
  Buffer pixels; // the pixel color to store the final image in RGB
  DrawFunc draw;

  // Implementation helpers
  void rasterize_mesh(const Mesh& mesh, const Mat4& trans);
  void rasterize_triangle(const Mesh::Triangle& tri, const Mat4& trans);

  void put_sample(int x, int y);

};


// HardRenderer ///////////////////////////////////////////////////////////////
// This render will need access to the OpenGL API
////
class HardRenerer : public Renderer {

};

} // namespace CEL

#endif
