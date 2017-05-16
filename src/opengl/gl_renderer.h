#ifndef CEL_OPENGL_GL_RENDERER_H
#define CEL_OPENGL_GL_RENDERER_H

#include <cstddef>

#include "../camera.h"
#include "../scene.h"
#include "../model.h"
#include "../renderer.h" // base class

#include "pixels.h" // WindowID

/*
 * opengl/renderer.h
 * Implement a OpenGL-based renderer.
 *
 * TODO: using OpenGL api instead of software rendering.
 * TODO: design some internal model format for rendering (see RTR)
 * TODO: implement fantastic CEL shading
 */

namespace CEL {

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

  // Default constructor
  GLRenderer();

  // Destructor
  ~GLRenderer() override {};

  // Render request
  void set_buffer_size(BufferSize width, BufferSize height) override;
  void render() override;

  // Implementation specific interface
  void set_window(WindowID w) { window = w; }

private:

  WindowID window; // set by GLViewer

  // Implementation helpers
  void rasterize_mesh(const Mesh& mesh, const Mat4& trans);
  void rasterize_triangle(const Mesh::Triangle& tri, const Mat4& trans);

};

} // namespace CEL

#endif
