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

  // Vertex Structure and Input Data Layout
  struct Vertex
  {
    Vertex() {}
    Vertex(float x, float y, float z,
      float r, float g, float b, float a,
      float nx, float ny, float nz
    ) : pos(x, y, z, 1), color(r, g, b, a), normal(nx, ny, nz) {}

    DirectX::XMFLOAT4 pos;  // DirectXMath use DirectX namespace 
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT3 normal;
  };

  // per-object constant-buffer layout 
  struct cbPerObject
  {
    DirectX::XMMATRIX WVP;  
    DirectX::XMMATRIX World;  

    cbPerObject() {}
    void update(const Mat4& wvp, const Mat4& world = Mat4::I()) {
      WVP = CEL_to_D3D(wvp);
      World = CEL_to_D3D(world);
    }
    static DirectX::XMMATRIX CEL_to_D3D(const Mat4& A) {
      return DirectX::XMMatrixSet( // must transpose
        A(0, 0), A(1, 0), A(2, 0), A(3, 0),
        A(0, 1), A(1, 1), A(2, 1), A(3, 1),
        A(0, 2), A(1, 2), A(2, 2), A(3, 2),
        A(0, 3), A(1, 3), A(2, 3), A(3, 3));
    }
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
  void set_d3d_interface(ID3D11Device* pdevice, ID3D11DeviceContext* pdevCon);
  void create_render_states();
  void compile_shader();

private:

  // D3D interfaces (own by the Viewer)
  ID3D11Device* d3d11Device;
  ID3D11DeviceContext* d3d11DevCon;

  // Default Shader objects
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
  Light g_light;  // global light-source data, fed to frame-buffer 
  cbPerFrame data_per_frame;  // memory-layouting for frame constant-buffer 


  // Implementation helpers //

  void add_mesh_buffer(const ModelInstance& modelIns);

  void render_meshes();
  void rasterize_triangle(const Mesh::Triangle& tri, const Mat4& trans);

  // Extra object for debug
  // FIXME 
  void set_default_scene();
  void render_default_scene();
  ID3D11Buffer* cubeIndexBuffer;
  ID3D11Buffer* cubeVertBuffer;
  ID3D11Buffer* cbPerObjectBuffer;
  cbPerObject g_cbPerObj;
  ID3D11InputLayout* vertLayout;

};

} // namespace CEL

#endif
