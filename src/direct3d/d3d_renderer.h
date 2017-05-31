#ifndef CEL_DIRECT3D_D3D_RENDERER_H
#define CEL_DIRECT3D_D3D_RENDERER_H

#include <cstddef>

#include "../algebra.h"
#include "../camera.h"
#include "../scene.h"
#include "../model.h"
#include "../renderer.h" // base class

#include <d3d11.h>
#include <d3dcompiler.h>  
#include <DirectXMath.h>  // d3d's math lib fits good with HLSL 

/*
* direct3d/d3d_renderer.h
* Implement a Direct3D11-based renderer.
*/

namespace CEL {

class D3DRenderer : public Renderer {

  // a private Vertex class; packing data for shader 
  struct VertexElement {
    DirectX::XMFLOAT4 pos;     // 4 float 
    DirectX::XMFLOAT4 color;  // 4 float 
    DirectX::XMFLOAT3 normal;  // 3 float 

    VertexElement(Vec4 p, Color c, Vec3 n) : 
      pos(p.x, p.y, p.z, p.h), 
      color(c.r, c.g, c.b, c.a), 
      normal(n.x, n.y, n.z) 
    {}
  };

  // per-object constant-buffer layout 
  struct cbPerObject
  {
    DirectX::XMMATRIX WVP;  // NOTE: row major storage 
    DirectX::XMMATRIX World;  

    cbPerObject(Mat4 wvp, Mat4 world = Mat4::I()) : 
      WVP(wvp(0,0), wvp(0,1), wvp(0,2), wvp(0,3), 
          wvp(1,0), wvp(1,1), wvp(1,2), wvp(1,3),
          wvp(2,0), wvp(2,1), wvp(2,2), wvp(2,3), 
          wvp(3,0), wvp(3,1), wvp(3,2), wvp(3,3) )
    { }
  };

  // a private Mesh class, ready for D3D rendering 
  class MeshBuffer {
  public:

    // Bound Mesh 
    const Mesh& model;

    // D3D buffers and related objects 
    ID3D11Buffer* meshIndexBuffer;
    ID3D11Buffer* meshVertBuffer;
    ID3D11InputLayout* vertLayout;
    ID3D11Buffer* cbPerObjectBuffer;

    MeshBuffer(const Mesh& mesh) : model(mesh) {};

    Mesh::size_type indices_num() const { 
      return model.get_triangles().size() * 3; }

  };

  // Over simple Light object, to debug
  struct Light
  {
    DirectX::XMFLOAT3 dir;
    float pad; // padding to match with shader's constant buffer packing scheme 
    DirectX::XMFLOAT4 ambient;
    DirectX::XMFLOAT4 diffuse;

    Light() { ZeroMemory(this, sizeof(Light)); }
  };

  // per-frame constant-buffer layout 
  struct cbPerFrame
  {
    Light light;
  };

public:

  // Default constructor
  D3DRenderer();

  // Destructor
  ~D3DRenderer() override;

  // Loading datas
  void set_scene(std::shared_ptr<const Scene> scene) override;

  // Render request
  void set_buffer_size(BufferSize width, BufferSize height) override;
  void render() override;

  // Implementation specific interface
  void set_d3d_interface(ID3D11Device* pdevice, ID3D11DeviceContext* pdevCon) {
    d3d11Device = pdevice; 
    d3d11DevCon = pdevCon;
    this->compile_shader(); // only runnable after get the device 
    this->create_render_states();
  }
  void compile_shader();
  void create_render_states();

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
  std::vector<MeshBuffer> mesh_buffers;
  ID3D11Buffer* cbPerFrameBuffer;  // buffer to hold frame-wide data 

  // Implementation helpers //

  void add_mesh_buffer(const ModelInstance& modelIns);

  void render_meshes();
  void rasterize_triangle(const Mesh::Triangle& tri, const Mat4& trans);

};

} // namespace CEL

#endif
