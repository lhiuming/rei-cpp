#ifndef REI_D3D_COMMON_RESOURCES_H
#define REI_D3D_COMMON_RESOURCES_H

#if DIRECT3D_ENABLED

#include <array>
#include <memory>

#include <DirectXMath.h>
#include <d3d11.h> // todo remove this
#include <d3d12.h>
#include <d3dx12.h>
#include <windows.h>
#include <wrl.h>

#include "../algebra.h"
#include "../color.h"
#include "../common.h"
#include "../renderer.h"

namespace rei {

namespace d3d {

using Microsoft::WRL::ComPtr;

class ViewportResources;


// TODO remove this hardcode class
struct cbPerObject {
  DirectX::XMMATRIX WVP; // DirectXMath fits well with HLSL
  DirectX::XMMATRIX World;

  cbPerObject() {}
  void update(const Mat4& wvp, const Mat4& world = Mat4::I()) {
    WVP = rei_to_D3D(wvp);
    World = rei_to_D3D(world);
  }
  static DirectX::XMMATRIX rei_to_D3D(const Mat4& A) {
    return DirectX::XMMatrixSet( // must transpose
      A(0, 0), A(1, 0), A(2, 0), A(3, 0), A(0, 1), A(1, 1), A(2, 1), A(3, 1), A(0, 2), A(1, 2),
      A(2, 2), A(3, 2), A(0, 3), A(1, 3), A(2, 3), A(3, 3));
  }
};

struct VertexElement {
  DirectX::XMFLOAT4 pos; // DirectXMath fits well with HLSL
  DirectX::XMFLOAT4 color;
  DirectX::XMFLOAT3 normal;

  VertexElement() {}
  VertexElement(const Vec4& p, const Color& c, const Vec3& n)
      : pos(p.x, p.y, p.z, p.h), color(c.r, c.g, c.b, c.a), normal(n.x, n.y, n.z) {}
  VertexElement(
    float x, float y, float z, float r, float g, float b, float a, float nx, float ny, float nz)
      : pos(x, y, z, 1), color(r, g, b, a), normal(nx, ny, nz) {}
};

// Over simple Light object, to debug
struct Light {
  DirectX::XMFLOAT3 dir;
  float pad; // padding to match with shader's constant buffer packing
  DirectX::XMFLOAT4 ambient;
  DirectX::XMFLOAT4 diffuse;

  Light() { ZeroMemory(this, sizeof(Light)); }
};

// per-frame constant-buffer layout
struct cbPerFrame {
  Light light;
};


struct ShaderCompileResult {
  ComPtr<ID3DBlob> vs_bytecode;
  ComPtr<ID3DBlob> ps_bytecode;
  D3D12_INPUT_LAYOUT_DESC input_layout;
};

struct RenderTargetSpec {
  DXGI_SAMPLE_DESC sample_desc; // multi-sampling parameters
  DXGI_FORMAT rt_format;
  DXGI_FORMAT ds_format;
  D3D12_DEPTH_STENCIL_VALUE ds_clear;
  RenderTargetSpec();
};

struct MeshData : rei::GeometryData {
  using GeometryData::GeometryData;
  ComPtr<ID3D12Resource> vert_buffer;
  ComPtr<ID3D12Resource> vert_upload_buffer;
  D3D12_VERTEX_BUFFER_VIEW vbv;

  ComPtr<ID3D12Resource> ind_buffer;
  ComPtr<ID3D12Resource> ind_upload_buffer;
  D3D12_INDEX_BUFFER_VIEW ibv;

  // D3D buffers and related objects
  ID3D11Buffer* meshIndexBuffer;
  ID3D11Buffer* meshVertBuffer;
  ID3D11Buffer* meshConstBuffer;
  cbPerObject mesh_cb_data;
};

struct ViewportData : rei::ViewportData {
  using rei::ViewportData::ViewportData;
  Mat4 view_proj = Mat4::I();
  std::array<FLOAT, 4> clear_color;
  D3D12_VIEWPORT d3d_viewport;
  D3D12_RECT scissor;
  std::weak_ptr<ViewportResources> viewport_resources;
};

struct ShaderData : rei::ShaderData {
  using rei::ShaderData::ShaderData;
  ShaderCompileResult compiled_data;
};

struct ModelData : rei::ModelData {
  // Bound aabb;
};

// Data proxy for all obejct in a scene
struct SceneData {};

// Wrap methods for an upload-type DX12 buffer
template <typename Ele>
class UploadBuffer : NoCopy {
public:
  UploadBuffer(ID3D12Device& device, UINT64 ele_num, bool is_const_buffer)
      : ele_num(ele_num), is_const_buffer(is_const_buffer) {
    // align the buffer size
    ele_bytesize = sizeof(Ele);
    if (is_const_buffer) { ele_bytesize = alightn_bytesize(ele_bytesize); }

    HRESULT hr;

    D3D12_HEAP_PROPERTIES heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE;
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(ele_bytesize * ele_num);
    D3D12_CLEAR_VALUE* clear_value = nullptr;
    hr = device.CreateCommittedResource(&heap_prop, heap_flags, &desc,
      D3D12_RESOURCE_STATE_GENERIC_READ, clear_value, IID_PPV_ARGS(&buffer));
    ASSERT(SUCCEEDED(hr));

    UINT subres = 0;
    D3D12_RANGE* range = nullptr;
    hr = buffer->Map(subres, range, (void**)(&mapped_memory));
    ASSERT(SUCCEEDED(hr));
  }
  ~UploadBuffer() {
    UINT subres = 0;
    D3D12_RANGE* range = nullptr;
    buffer->Unmap(subres, range);
  }

  static inline UINT64 align_cb_bytesize(UINT64 bytesize) {
    const UINT64 CB_ALIGNMENT_WIDTH = 256;
    const UINT64 CB_ALIGNMENT_MASK = ~255;
    return (bytesize + CB_ALIGNMENT_WIDTH) & CB_ALIGNMENT_MASK;
  }

private:
  UINT ele_num;
  UINT ele_bytesize;
  bool is_const_buffer;

  ComPtr<ID3D12Resource> buffer;
  byte* mapped_memory;
};

} // namespace d3d

} // namespace rei

#endif

#endif
