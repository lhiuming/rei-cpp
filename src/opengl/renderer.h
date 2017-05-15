#ifndef CEL_OPENGL_RENDERER_H
#define CEL_OPENGL_RENDERER_H

#include <cstddef>

#include "../camera.h"
#include "../scene.h"
#include "../model.h"

#include "../renderer.h" // base class

/*
 * opengl/renderer.h
 * Implement a OpenGL-based renderer.
 *
 * TODO: using OpenGL api instead of software rendering.
 * TODO: design some internal model format for rendering (see RTR)
 * TODO: implement fantastic CEL shading
 * TODO: implement a hard renderer
 */

namespace CEL {

// SoftRenderer ///////////////////////////////////////////////////////////////
// The software renderering; everything is done on CPU and main memory.
////

class GLRenderer : public Renderer {

  // a private Vertex class
  class Vertex {

  };

  // a private Triangle class; represent a primitive
  class Triangle {

  };

public:

  // Type Alias
  using Buffer = unsigned char*;
  using DrawFunc = std::function<void (Buffer, BufferSize, BufferSize)>;

  // Default constructor
  GLRenderer();

  // Destructor
  ~GLRenderer() override {};

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


} // namespace CEL

#endif
