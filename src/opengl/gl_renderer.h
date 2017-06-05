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

  // Holds GL objects related to a mesh
  // TODO: make a more automonous class (auto alloc/release GL resouces)
  struct BufferedMesh {
    const Mesh& mesh;
    GLuint meshVAO;
    GLuint meshIndexBuffer;
    GLuint meshVertexBuffer;

    BufferedMesh(const Mesh& m) : mesh(m) {};

    Mesh::size_type indices_num() const {
      return mesh.get_triangles().size() * 3; }
  };

public:

  // Type Alias
  using Buffer = unsigned char*;

  // Default constructor
  GLRenderer();

  // Destructor
  ~GLRenderer() override;

  // Configurations
  void set_scene(std::shared_ptr<const Scene> scene) override;

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

  // Rendering objects
  std::vector<BufferedMesh> meshes;

  // Implementation helpers
  void add_buffered_mesh(const Mesh& mesh, const Mat4& trans);
  void render_mesh(BufferedMesh& buffered_mesh);

};

} // namespace CEL

#endif
