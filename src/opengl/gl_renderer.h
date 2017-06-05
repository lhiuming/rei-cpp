#ifndef CEL_OPENGL_GL_RENDERER_H
#define CEL_OPENGL_GL_RENDERER_H

#include <cstddef>

#include <GL/glew.h>    // it includes GL; must before glfw
#include <GLFW/glfw3.h>

#include "../camera.h"
#include "../scene.h"
#include "../model.h"
#include "../renderer.h" // base class

/*
 * opengl/renderer.h
 * Implement a OpenGL-based renderer.
 *
 * TODO: implement fantastic CEL shading
 * TODO: design some internal model format for rendering (for more fancy fx)
 */

namespace CEL {

class GLRenderer : public Renderer {

  // TODO a private Vertex class
  class Vertex { };

  // TODO a private Triangle class; represent a primitive
  class Triangle { };

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
  void set_gl_context(GLFWwindow* w);
  void compile_shader();

private:

  // GL interface object
  GLFWwindow* window; // manged by GLViewer
  GLuint program; // object id for a unified pass-through shader

  // Implementation helpers
  void rasterize_mesh(const Mesh& mesh, const Mat4& trans);

};

} // namespace CEL

#endif
