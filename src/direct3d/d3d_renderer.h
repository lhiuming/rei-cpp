#ifndef CEL_DIRECT3D_D3D_RENDERER_H
#define CEL_DIRECT3D_D3D_RENDERER_H

#include <cstddef>

#include "../camera.h"
#include "../scene.h"
#include "../model.h"
#include "../renderer.h" // base class

/*
* direct3d/d3d_renderer.h
* Implement a Direct3D11-based renderer.
*/

namespace CEL {

  class D3DRenderer : public Renderer {

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
    D3DRenderer();

    // Destructor
    ~D3DRenderer() override {};

    // Render request
    void set_buffer_size(BufferSize width, BufferSize height) override;
    void render() override;

    // Implementation specific interface
    //void set_window(WindowID w) { window = w; }

  private:

    //WindowID window; // set by GLViewer
    //GLuint program; // object id for a unified pass-through shader

                    // Implementation helpers
    void rasterize_mesh(const Mesh& mesh, const Mat4& trans);
    void rasterize_triangle(const Mesh::Triangle& tri, const Mat4& trans);

  };

} // namespace CEL

#endif
