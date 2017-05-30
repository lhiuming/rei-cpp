#ifndef CEL_DIRECT3D_D3D_RENDERER_H
#define CEL_DIRECT3D_D3D_RENDERER_H

#include <cstddef>

#include "../camera.h"
#include "../scene.h"
#include "../model.h"
#include "../renderer.h" // base class

#include <d3d11.h>
#include <d3dcompiler.h>  

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

  // Default constructor
  D3DRenderer();

  // Destructor
  ~D3DRenderer() override;

  // Render request
  void set_buffer_size(BufferSize width, BufferSize height) override;
  void render() override;

  // Implementation specific interface
  void set_d3d_interface(ID3D11Device* pdevice, ID3D11DeviceContext* pdevCon) {
    d3d11Device = pdevice; 
    d3d11DevCon = pdevCon;
    this->compile_shader(); // only runnable after get the device 
  }
  void compile_shader();

private:

  // D3D interfaces (own by the Viewer)
  ID3D11Device* d3d11Device;
  ID3D11DeviceContext* d3d11DevCon;

  // Shader objects
  ID3D11VertexShader* VS;
  ID3D11PixelShader* PS;
  ID3DBlob* VS_Buffer;
  ID3DBlob* PS_Buffer;

  // Pipeline states objects
  ID3D11RasterizerState* FaceRender;  // normal 
  ID3D11RasterizerState* LineRender;  // with depth bias (to draw cel-line) 

  // Rendering objects 
  ID3D11Buffer* meshIndexBuffer;
  ID3D11Buffer* meshVertBuffer;
  ID3D11InputLayout* vertLayout;

  ID3D11Buffer* cbPerObjectBuffer; // buffer to store per-object data 
  ID3D11Buffer* cbPerFrameBuffer;  // buffer to hold frame-wide data 

  // Implementation helpers //

  void rasterize_mesh(const Mesh& mesh, const Mat4& trans);
  void rasterize_triangle(const Mesh::Triangle& tri, const Mat4& trans);

};

} // namespace CEL

#endif
