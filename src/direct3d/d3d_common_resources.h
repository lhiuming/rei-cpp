#ifndef REI_D3D_COMMON_RESOURCES_H
#define REI_D3D_COMMON_RESOURCES_H

#if DIRECT3D_ENABLED

#include <array>
#include <memory>

#include <DirectXMath.h>
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

// Wrap methods for an upload-type DX12 buffer
template <typename Ele>
class UploadBuffer : NoCopy {
public:
  struct Hasher {
    std::size_t operator()(const UploadBuffer& value) const { return std::uintptr_t(value.resource()); }
  };

  UploadBuffer(ID3D12Device& device, UINT64 element_num, bool is_const_buffer)
      : m_element_num(element_num), m_is_const_buffer(is_const_buffer) {
    // align the buffer size
    m_element_bytesize = sizeof(Ele);
    if (is_const_buffer) { m_element_bytesize = align_cb_bytesize(m_element_bytesize); }

    HRESULT hr;

    D3D12_HEAP_PROPERTIES heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE;
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(m_element_bytesize * element_num);
    D3D12_CLEAR_VALUE* clear_value = nullptr;
    hr = device.CreateCommittedResource(&heap_prop, heap_flags, &desc,
      D3D12_RESOURCE_STATE_GENERIC_READ, clear_value, IID_PPV_ARGS(&buffer));
    REI_ASSERT(SUCCEEDED(hr));

    UINT subres = 0;
    D3D12_RANGE* range = nullptr;
    hr = buffer->Map(subres, range, (void**)(&mapped_memory));
    REI_ASSERT(SUCCEEDED(hr));
  }

  ~UploadBuffer() {
    UINT subres = 0;
    D3D12_RANGE* range = nullptr;
    buffer->Unmap(subres, range);
  }

  bool operator==(const UploadBuffer& other) const { return buffer == other.buffer; }

  bool is_const_buffer() const { return m_is_const_buffer; }
  UINT size() const { return m_element_num; }
  UINT element_bytesize() const { return m_element_bytesize; }

  void update(const Ele& value, UINT index = 0) const {
    REI_ASSERT(index < m_element_num);
    memcpy(mapped_memory + index * m_element_bytesize, &value, sizeof(Ele));
  }

  ID3D12Resource* resource() const { return buffer.Get(); }
  D3D12_GPU_VIRTUAL_ADDRESS buffer_address(UINT element_index = 0) const {
    REI_ASSERT(element_index < m_element_num);
    return buffer.Get()->GetGPUVirtualAddress() + element_index * m_element_bytesize;
  }

  static inline UINT64 align_cb_bytesize(UINT64 bytesize) {
    const UINT64 CB_ALIGNMENT_WIDTH = 256;
    const UINT64 CB_ALIGNMENT_MASK = ~255;
    return (bytesize + CB_ALIGNMENT_WIDTH) & CB_ALIGNMENT_MASK;
  }

private:
  UINT m_element_num;
  UINT m_element_bytesize;
  bool m_is_const_buffer;

  ComPtr<ID3D12Resource> buffer;
  byte* mapped_memory;
};

inline static DirectX::XMMATRIX rei_to_D3D(const Mat4& A) {
  return DirectX::XMMatrixSet( // must transpose
    A(0, 0), A(1, 0), A(2, 0), A(3, 0), A(0, 1), A(1, 1), A(2, 1), A(3, 1), A(0, 2), A(1, 2),
    A(2, 2), A(3, 2), A(0, 3), A(1, 3), A(2, 3), A(3, 3));
}

// TODO remove this hardcode class
struct cbPerObject {
  DirectX::XMMATRIX WVP; // DirectXMath fits well with HLSL
  DirectX::XMMATRIX World;

  void update(const Mat4& wvp, const Mat4& world = Mat4::I()) {
    WVP = rei_to_D3D(wvp);
    World = rei_to_D3D(world);
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
  DirectX::XMMATRIX camera_world_trans;
  DirectX::XMFLOAT3 camera_pos;

  void set_camera_world_trans(const Mat4& m) { camera_world_trans = rei_to_D3D(m); }
  void set_camera_pos(const Vec3& v) { camera_pos = {float(v.x), float(v.y), float(v.z)}; }
};

struct RenderTargetSpec {
  DXGI_SAMPLE_DESC sample_desc; // multi-sampling parameters
  DXGI_FORMAT rt_format;
  DXGI_FORMAT ds_format;
  D3D12_DEPTH_STENCIL_VALUE ds_clear;
  RenderTargetSpec();

  bool operator==(const RenderTargetSpec& other) const {
    return sample_desc.Count == other.sample_desc.Count
           && sample_desc.Quality == other.sample_desc.Quality && rt_format == other.rt_format
           && ds_format == other.ds_format && ds_clear.Depth == other.ds_clear.Depth
           && ds_clear.Stencil == other.ds_clear.Stencil;
  }

  std::size_t simple_hash() const {
    // 5 bits
    std::size_t hash_sample = sample_desc.Count ^ (sample_desc.Quality << 4);
    // 16 bits
    std::size_t hash_rt_ds_format = rt_format ^ (ds_format << 8);
    // ignored
    std::size_t hash_ds_clear = 0;
    return hash_sample ^ (hash_rt_ds_format << 5);
  }
};

struct GeometryData : BaseGeometryData {
  using BaseGeometryData::BaseGeometryData;
};

struct MeshData : GeometryData {
  using GeometryData::GeometryData;
  ComPtr<ID3D12Resource> vert_buffer;
  ComPtr<ID3D12Resource> vert_upload_buffer;
  D3D12_VERTEX_BUFFER_VIEW vbv;

  ComPtr<ID3D12Resource> ind_buffer;
  ComPtr<ID3D12Resource> ind_upload_buffer;
  D3D12_INDEX_BUFFER_VIEW ibv;
  UINT index_num;
};

class ViewportResources;

struct ViewportData : BaseViewportData {
  using BaseViewportData::BaseViewportData;
  Vec3 pos = {};
  Mat4 view = Mat4::I();
  Mat4 proj = Mat4::I();
  Mat4 view_proj = Mat4::I();
  std::array<FLOAT, 4> clear_color;
  D3D12_VIEWPORT d3d_viewport;
  D3D12_RECT scissor;
  std::weak_ptr<ViewportResources> viewport_resources;

  void update_camera_transform(const Camera& cam) { 
    pos = cam.position();
    view = cam.view(Handness::Right, Handness::Left, VectorTarget::Row);
    proj = cam.project(Handness::Left, Handness::Left, VectorTarget::Row);
    view_proj = cam.view_proj(Handness::Right, Handness::Left, VectorTarget::Row);
  }
};

struct ShaderCompileResult {
  ComPtr<ID3DBlob> vs_bytecode;
  ComPtr<ID3DBlob> ps_bytecode;
  std::vector<D3D12_INPUT_ELEMENT_DESC> vertex_input_descs;
};

struct ShaderConstBuffers {
  std::unique_ptr<UploadBuffer<cbPerFrame>> per_frame_CB;
  std::shared_ptr<UploadBuffer<cbPerObject>> per_object_CBs;
  // TODO more delicated management
  UINT next_object_index;
};

struct ShaderData : BaseShaderData {
  using BaseShaderData::BaseShaderData;
  ShaderCompileResult compiled_data;
  ShaderConstBuffers const_buffers;
  ComPtr<ID3D12RootSignature> root_signature;
};

struct MaterialData : BaseMaterialData {
  using BaseMaterialData::BaseMaterialData;
  std::shared_ptr<ShaderData> shader;
};

struct ModelData : BaseModelData {
  using BaseModelData::BaseModelData;
  // Bound aabb;
  std::shared_ptr<GeometryData> geometry;
  std::shared_ptr<MaterialData> material;
  UINT const_buffer_index; // in shader cb buffer
  UINT cbv_index; // in device shared heap
};

// Data proxy for all obejct in a scene
struct SceneData {};

struct CullingData : BaseCullingData {
  using BaseCullingData::BaseCullingData;
  std::vector<ModelData> models;
};

} // namespace d3d

} // namespace rei

#endif

#endif
