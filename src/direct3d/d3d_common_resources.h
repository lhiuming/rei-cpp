#ifndef REI_D3D_COMMON_RESOURCES_H
#define REI_D3D_COMMON_RESOURCES_H

#if DIRECT3D_ENABLED

#include <array>
#include <memory>

#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_4.h>
#include <windows.h>
#include <wrl.h>

#include "../algebra.h"
#include "../camera.h"
#include "../color.h"
#include "../common.h"
#include "../model.h"
#include "../renderer.h"
#include "d3d_utils.h"

namespace rei {

namespace d3d {

using Microsoft::WRL::ComPtr;

// Some contants
constexpr DXGI_FORMAT c_index_format = DXGI_FORMAT_R16_UINT;
constexpr DXGI_FORMAT c_accel_struct_vertex_pos_format = DXGI_FORMAT_R32G32B32_FLOAT;

// Reminder: using right-hand coordinate throughout the pipeline
constexpr bool is_right_handed = true;

inline static constexpr DXGI_FORMAT to_dxgi_format(ResourceFormat format) {
  switch (format) {
    case ResourceFormat::R32G32B32A32_FLOAT:
      return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case ResourceFormat::B8G8R8A8_UNORM:
      return DXGI_FORMAT_R8G8B8A8_UNORM;
    case ResourceFormat::D24_UNORM_S8_UINT:
      return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case ResourceFormat::AcclerationStructure:
      return DXGI_FORMAT_UNKNOWN;
    case ResourceFormat::Count:
    default:
      REI_ERROR("Unhandled format");
      return DXGI_FORMAT_UNKNOWN;
  }
}

inline static constexpr D3D12_SRV_DIMENSION to_srv_dimension(ResourceDimension dim) {
  switch (dim) {
    case ResourceDimension::Undefined:
    case ResourceDimension::Raw:
      return D3D12_SRV_DIMENSION_UNKNOWN;
    case ResourceDimension::StructuredBuffer:
      return D3D12_SRV_DIMENSION_BUFFER;
    case ResourceDimension::Texture1D:
      return D3D12_SRV_DIMENSION_TEXTURE1D;
    case ResourceDimension::Texture1DArray:
      return D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
    case ResourceDimension::Texture2D:
      return D3D12_SRV_DIMENSION_TEXTURE2D;
    case ResourceDimension::Texture2DArray:
      return D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    case ResourceDimension::Texture3D:
      return D3D12_SRV_DIMENSION_TEXTURE3D;
    case ResourceDimension::AccelerationStructure:
      return D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    default:
      REI_ERROR("Unhandled case detected");
      return D3D12_SRV_DIMENSION_UNKNOWN;
  }
}

inline static constexpr D3D12_UAV_DIMENSION to_usv_dimension(ResourceDimension dim) {
  switch (dim) {
    case rei::ResourceDimension::Undefined:
    case rei::ResourceDimension::Raw:
      return D3D12_UAV_DIMENSION_UNKNOWN;
    case rei::ResourceDimension::StructuredBuffer:
      return D3D12_UAV_DIMENSION_BUFFER;
      break;
    case rei::ResourceDimension::Texture1D:
      return D3D12_UAV_DIMENSION_TEXTURE1D;
    case rei::ResourceDimension::Texture1DArray:
      return D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
    case rei::ResourceDimension::Texture2D:
      return D3D12_UAV_DIMENSION_TEXTURE2D;
    case rei::ResourceDimension::Texture2DArray:
      return D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
    case rei::ResourceDimension::Texture3D:
      return D3D12_UAV_DIMENSION_TEXTURE3D;
    case rei::ResourceDimension::AccelerationStructure:
      return D3D12_UAV_DIMENSION_UNKNOWN;
    default:
      REI_ERROR("Unhandled case detected");
      return D3D12_UAV_DIMENSION_UNKNOWN;
  }
}

inline static DirectX::XMFLOAT4 rei_to_D3D(const Vec4& v) {
  return DirectX::XMFLOAT4(v.x, v.y, v.z, v.h);
}

inline static DirectX::XMMATRIX rei_to_D3D(const Mat4& A) {
  // REMARK: Mat4 is column major, while D3D is row major by default
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
  ResourceFormat rt_format;
  ResourceFormat ds_format;
  DXGI_FORMAT dxgi_rt_format;
  DXGI_FORMAT dxgi_ds_format;
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
    std::size_t hash_rt_ds_format = dxgi_rt_format ^ (dxgi_ds_format << 8);
    // ignored
    std::size_t hash_ds_clear = 0;
    return hash_sample ^ (hash_rt_ds_format << 5);
  }
};

class ViewportResources;
struct SwapchainData : BaseSwapchainData {
  using BaseSwapchainData::BaseSwapchainData;
  // TODO deelete the viewport-resource class
  std::shared_ptr<ViewportResources> res;
};

struct BufferData : BaseBufferData {
  using BaseBufferData::BaseBufferData;
  D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
};

struct DefaultBufferData : BufferData {
  using BufferData::BufferData;
  ComPtr<ID3D12Resource> buffer;
  DefaultBufferFormat meta;
};

struct ConstBufferData : BufferData {
  using BufferData::BufferData;
  std::unique_ptr<UploadBuffer> buffer;
  ConstBufferLayout layout;
};

struct GeometryData : BaseGeometryData {
  using BaseGeometryData::BaseGeometryData;
};

struct MeshData : GeometryData {
  using GeometryData::GeometryData;
  ComPtr<ID3D12Resource> vert_buffer;
  ComPtr<ID3D12Resource> vert_upload_buffer;
  D3D12_VERTEX_BUFFER_VIEW vbv = {NULL, UINT_MAX, UINT_MAX};
  UINT vertex_num = UINT_MAX;

  CD3DX12_GPU_DESCRIPTOR_HANDLE vert_srv_gpu = {D3D12_DEFAULT};
  CD3DX12_CPU_DESCRIPTOR_HANDLE vert_srv_cpu = {D3D12_DEFAULT};
  DXGI_FORMAT vertex_pos_format;

  ComPtr<ID3D12Resource> ind_buffer;
  ComPtr<ID3D12Resource> ind_upload_buffer;
  D3D12_INDEX_BUFFER_VIEW ibv = {NULL, UINT_MAX, DXGI_FORMAT_UNKNOWN};
  UINT index_num = UINT_MAX;

  CD3DX12_GPU_DESCRIPTOR_HANDLE ind_srv_gpu = {D3D12_DEFAULT};
  CD3DX12_CPU_DESCRIPTOR_HANDLE ind_srv_cpu = {D3D12_DEFAULT};
  DXGI_FORMAT index_format = DXGI_FORMAT_UNKNOWN;

  ComPtr<ID3D12Resource> blas_buffer;
  ComPtr<ID3D12Resource> scratch_buffer;
};

class ViewportResources;

struct ViewportData : BaseScreenTransformData {
  using BaseScreenTransformData::BaseScreenTransformData;
  Vec3 pos = {};
  Mat4 view = Mat4::I();
  Mat4 view_proj = Mat4::I();
  std::array<FLOAT, 4> clear_color;
  D3D12_VIEWPORT d3d_viewport;
  D3D12_RECT scissor;

  void update_camera_transform(const Camera& cam) {
    REI_ASSERT(is_right_handed);
    pos = cam.position();
    view = cam.view();
    view_proj = cam.view_proj_halfz();
  }
};

struct RootSignatureDescMemory {
  CD3DX12_ROOT_SIGNATURE_DESC desc = CD3DX12_ROOT_SIGNATURE_DESC(D3D12_DEFAULT);
  // probably no more than 4 space
  using ParamContainer = v_array<CD3DX12_ROOT_PARAMETER, 4>;
  ParamContainer params;
  // probably no more than 4 space * 4 range-types
  using RangeContainer = v_array<CD3DX12_DESCRIPTOR_RANGE, 16>;
  RangeContainer ranges;
};

struct RasterizationShaderMetaInfo {
  CD3DX12_RASTERIZER_DESC raster_state = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  CD3DX12_DEPTH_STENCIL_DESC depth_stencil = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  bool is_depth_stencil_null = false;
  CD3DX12_BLEND_DESC blend_state = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  CD3DX12_ROOT_SIGNATURE_DESC root_desc = CD3DX12_ROOT_SIGNATURE_DESC(D3D12_DEFAULT);
  D3D12_INPUT_LAYOUT_DESC input_layout = {nullptr, 0};

  RasterizationShaderMetaInfo() {
    REI_ASSERT(is_right_handed);
    raster_state.FrontCounterClockwise = true;
    depth_stencil.DepthFunc
      = D3D12_COMPARISON_FUNC_GREATER; // we use right-hand coordiante throughout the pipeline
  }
};

struct RayTracingShaderMetaInfo {
  RootSignatureDescMemory global;
  std::wstring hitgroup_name;
  std::wstring closest_hit_name;
  // std::wstring any_hit_name;
  // std::wstring intersection_name;
  RootSignatureDescMemory hitgroup;
  std::wstring raygen_name;
  RootSignatureDescMemory raygen;
  std::wstring miss_name;
  RootSignatureDescMemory miss;

  RayTracingShaderMetaInfo() {}

  RayTracingShaderMetaInfo(rei::RaytracingShaderMetaInfo&& meta)
      : hitgroup_name(std::move(meta.hitgroup_name)),
        closest_hit_name(std::move(meta.closest_hit_name)),
        raygen_name(std::move(meta.raygen_name)),
        miss_name(std::move(meta.miss_name)) {
    fill_signature(global, meta.global_signature, true);
    fill_signature(hitgroup, meta.hitgroup_signature);
    fill_signature(raygen, meta.raygen_signature);
    fill_signature(miss, meta.miss_signature);
  }

  // Convert high-level shader signature to D3D12 Root Signature Desc
  inline void fill_signature(
    RootSignatureDescMemory& desc, const ShaderSignature& signature, bool global = false) {
    auto& param_table = signature.param_table;

    auto& ranges = desc.ranges;
    ranges.clear();
    auto& params_descs = desc.params;
    params_descs.clear();

    for (int space = 0; space < param_table.size(); space++) {
      auto& params = param_table[space];
      size_t range_offset = ranges.size();

      if (params.const_buffers.size())
        ranges.emplace_back(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, params.const_buffers.size(), 0, space);
      if (params.shader_resources.size())
        ranges.emplace_back(
          D3D12_DESCRIPTOR_RANGE_TYPE_SRV, params.shader_resources.size(), 0, space);
      if (params.unordered_accesses.size())
        ranges.emplace_back(
          D3D12_DESCRIPTOR_RANGE_TYPE_UAV, params.unordered_accesses.size(), 0, space);
      if (params.samplers.size())
        ranges.emplace_back(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, params.samplers.size(), 0, space);

      UINT new_range_count = UINT(ranges.size() - range_offset);
      if (new_range_count > 0) {
        params_descs.emplace_back().InitAsDescriptorTable(new_range_count, &ranges[range_offset]);
      }
    }

    D3D12_ROOT_SIGNATURE_FLAGS flags
      = global ? D3D12_ROOT_SIGNATURE_FLAG_NONE : D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    desc.desc.Init(params_descs.size(), params_descs.data(), 0, nullptr, flags);
  }
};

struct ShaderCompileResult {
  ComPtr<ID3DBlob> vs_bytecode;
  ComPtr<ID3DBlob> ps_bytecode;
};

struct ShaderConstBuffers {
  std::unique_ptr<UploadBuffer> per_frame_CB;
  std::shared_ptr<UploadBuffer> per_object_CBs;
  // TODO more delicated management
  UINT next_object_index = UINT_MAX;
};

struct ShaderData : BaseShaderData {
  using BaseShaderData::BaseShaderData;
  ComPtr<ID3D12RootSignature> root_signature;
};

struct ShaderArgumentData : BaseShaderArgument {
  using BaseShaderArgument::BaseShaderArgument;

  // support up to 4 root cbv
  v_array<D3D12_GPU_VIRTUAL_ADDRESS, 4> cbvs;

  D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor_gpu = {0};
  D3D12_CPU_DESCRIPTOR_HANDLE base_descriptor_cpu = {0};
  UINT alloc_index = -1;
  UINT alloc_num = -1;
};

struct RasterizationShaderData : ShaderData {
  using ShaderData::ShaderData;
  std::unique_ptr<RasterizationShaderMetaInfo> meta;
  ShaderCompileResult compiled_data;
  [[deprecated]] ShaderConstBuffers const_buffers;
};

struct RaytracingShaderData : ShaderData {
  using ShaderData::ShaderData;
  RayTracingShaderMetaInfo meta;

  struct LocalShaderData {
    ComPtr<ID3D12RootSignature> root_signature;
    const void* shader_id;
  };
  LocalShaderData raygen;
  LocalShaderData hitgroup;
  LocalShaderData miss;

  ComPtr<ID3D12StateObject> pso;
};

struct RaytracingShaderTableData : BaseBufferData {
  using BaseBufferData ::BaseBufferData;
  std::shared_ptr<RaytracingShaderData> shader;
  std::shared_ptr<ShaderTable> raygen;
  std::shared_ptr<ShaderTable> hitgroup;
  std::shared_ptr<ShaderTable> miss;
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
  UINT const_buffer_index = UINT_MAX; // in shader cb buffer
  UINT cbv_index = UINT_MAX;          // in device shared heap

  UINT tlas_instance_id = UINT_MAX;

  void update_transform(Model& model) {
    // NOTE: check how ViewportData store the transforms
    REI_ASSERT(is_right_handed);
    this->transform = model.get_transform();
  }
};

struct CullingData : BaseCullingData {
  using BaseCullingData::BaseCullingData;
  std::vector<ModelData> models;
};

} // namespace d3d

} // namespace rei

#endif

#endif
