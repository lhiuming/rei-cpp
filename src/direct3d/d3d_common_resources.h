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

inline static constexpr DXGI_FORMAT to_dxgi_format_srv(ResourceFormat format) {
  if (format == ResourceFormat::D24_UNORM_S8_UINT) { return DXGI_FORMAT_R24_UNORM_X8_TYPELESS; }
  return to_dxgi_format(format);
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
    case ResourceDimension::Undefined:
    case ResourceDimension::Raw:
      return D3D12_UAV_DIMENSION_UNKNOWN;
    case ResourceDimension::StructuredBuffer:
      return D3D12_UAV_DIMENSION_BUFFER;
      break;
    case ResourceDimension::Texture1D:
      return D3D12_UAV_DIMENSION_TEXTURE1D;
    case ResourceDimension::Texture1DArray:
      return D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
    case ResourceDimension::Texture2D:
      return D3D12_UAV_DIMENSION_TEXTURE2D;
    case ResourceDimension::Texture2DArray:
      return D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
    case ResourceDimension::Texture3D:
      return D3D12_UAV_DIMENSION_TEXTURE3D;
    case ResourceDimension::AccelerationStructure:
      return D3D12_UAV_DIMENSION_UNKNOWN;
    default:
      REI_ERROR("Unhandled case detected");
      return D3D12_UAV_DIMENSION_UNKNOWN;
  }
}

inline static constexpr D3D12_RESOURCE_STATES to_res_state(ResourceState state) {
  switch (state) {
    case ResourceState::Present:
      return D3D12_RESOURCE_STATE_PRESENT;
    case ResourceState::RenderTarget:
      return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case ResourceState::DeptpWrite:
      return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case ResourceState::PixelShaderResource:
      return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case ResourceState::UnorderedAccess:
      return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    default:
      REI_THROW("Unhandled resource state");
      return D3D12_RESOURCE_STATE_COMMON;
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

constexpr UINT c_input_layout_num = 3;
extern const D3D12_INPUT_ELEMENT_DESC c_input_layout[3];

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
  virtual ID3D12Resource* get_res() const = 0;
};

// Texture
struct DefaultBufferData : BufferData {
  using BufferData::BufferData;
  ComPtr<ID3D12Resource> buffer;
  DefaultBufferFormat meta;

  ID3D12Resource* get_res() const { return buffer.Get(); }
};

// Buffer
struct ConstBufferData : BufferData {
  using BufferData::BufferData;
  std::unique_ptr<UploadBuffer> buffer;
  ConstBufferLayout layout;
  ID3D12Resource* get_res() const { return buffer->resource(); }
};

struct GeometryData : BaseGeometryData {
  using BaseGeometryData::BaseGeometryData;
};

struct MeshData : GeometryData {
  using GeometryData::GeometryData;
  bool is_dummy = false;
  ComPtr<ID3D12Resource> vert_buffer;
  ComPtr<ID3D12Resource> vert_upload_buffer;
  D3D12_VERTEX_BUFFER_VIEW vbv = {NULL, UINT_MAX, UINT_MAX};
  UINT vertex_num = UINT_MAX;

  CD3DX12_GPU_DESCRIPTOR_HANDLE vert_srv_gpu = {D3D12_DEFAULT};
  CD3DX12_CPU_DESCRIPTOR_HANDLE vert_srv_cpu = {D3D12_DEFAULT};

  ComPtr<ID3D12Resource> ind_buffer;
  ComPtr<ID3D12Resource> ind_upload_buffer;
  D3D12_INDEX_BUFFER_VIEW ibv = {NULL, UINT_MAX, DXGI_FORMAT_UNKNOWN};
  UINT index_num = UINT_MAX;

  CD3DX12_GPU_DESCRIPTOR_HANDLE ind_srv_gpu = {D3D12_DEFAULT};
  CD3DX12_CPU_DESCRIPTOR_HANDLE ind_srv_cpu = {D3D12_DEFAULT};

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
  FLOAT clear_depth;
  FLOAT clear_stencil;
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
  using ParamContainer = FixedVec<CD3DX12_ROOT_PARAMETER, 4>;
  ParamContainer param_memory;
  // probably no more than 4 space * 4 range-types
  using RangeContainer = FixedVec<CD3DX12_DESCRIPTOR_RANGE, 16>;
  RangeContainer range_memory;
  // probably no more than 8 static sampler
  using StaticSamplerContainer = FixedVec<CD3DX12_STATIC_SAMPLER_DESC, 8>;
  StaticSamplerContainer static_sampler_memory;

  RootSignatureDescMemory(const RootSignatureDescMemory& other) = delete;
  RootSignatureDescMemory(RootSignatureDescMemory&& other) = delete;

  // Convert high-level shader signature to D3D12 Root Signature Desc
  inline void init_signature(const ShaderSignature& signature, bool local) {
    auto& param_table = signature.param_table;

    range_memory.clear();
    param_memory.clear();

    for (int space = 0; space < param_table.size(); space++) {
      auto& params = param_table[space];
      size_t range_offset = range_memory.size();

      // CBV/SRV/UAVs
      if (params.const_buffers.size())
        range_memory.emplace_back(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, params.const_buffers.size(), 0, space);
      if (params.shader_resources.size())
        range_memory.emplace_back(
          D3D12_DESCRIPTOR_RANGE_TYPE_SRV, params.shader_resources.size(), 0, space);
      if (params.unordered_accesses.size())
        range_memory.emplace_back(
          D3D12_DESCRIPTOR_RANGE_TYPE_UAV, params.unordered_accesses.size(), 0, space);

      // Sampler
      // TODO support sampler
      // TODO check that sampler is in standalone heap 
      if (params.samplers.size())
        range_memory.emplace_back(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, params.samplers.size(), 0, space);

      UINT new_range_count = UINT(range_memory.size() - range_offset);
      if (new_range_count > 0) {
        param_memory.emplace_back().InitAsDescriptorTable(new_range_count, &range_memory[range_offset]);
      }

      // Static Sampler
      for (int i = 0; i < params.static_samplers.size(); i++) {
        //auto sampler = params.static_samplers[i];
        auto& sampler_desc = static_sampler_memory.emplace_back();
        sampler_desc.Init(i, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT);
        sampler_desc.RegisterSpace = space;
      }
    }

    D3D12_ROOT_SIGNATURE_FLAGS flags
      = local ? D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE : D3D12_ROOT_SIGNATURE_FLAG_NONE;
    if (param_memory.size() > 0 || static_sampler_memory.size() > 0)
      desc.Init(param_memory.size(), param_memory.data(), static_sampler_memory.size(),
        static_sampler_memory.data(), flags);
  }
};

struct RasterizationShaderMetaDesc {
  CD3DX12_RASTERIZER_DESC raster_state = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  CD3DX12_DEPTH_STENCIL_DESC depth_stencil = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  bool is_depth_stencil_null = false;
  CD3DX12_BLEND_DESC blend_state = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

  CD3DX12_STATIC_SAMPLER_DESC static_sampler_desc {};
  RootSignatureDescMemory root_signature {};

    // TODO allow configuration of input layout
  D3D12_INPUT_LAYOUT_DESC input_layout = {c_input_layout, c_input_layout_num};

  FixedVec<DXGI_FORMAT, 8> rt_formats;

  RasterizationShaderMetaDesc() {
    REI_ASSERT(is_right_handed);
    raster_state.FrontCounterClockwise = true;
    depth_stencil.DepthFunc
      = D3D12_COMPARISON_FUNC_GREATER; // we use right-hand coordiante throughout the pipeline
  }

  RasterizationShaderMetaDesc(RasterizationShaderMetaInfo&& meta) { 
    this->init(std::move(meta));
  }

  void init(RasterizationShaderMetaInfo&& meta) {
    root_signature.init_signature(meta.signature, false);
    root_signature.desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    for (auto& rt_desc : meta.render_target_descs) {
      rt_formats.push_back(to_dxgi_format(rt_desc.format));
    }
    is_depth_stencil_null = meta.is_depth_stencil_disabled;
  }

  UINT get_rtv_formats(DXGI_FORMAT (&dest)[8]) const { 
    for (int i = 0; i < rt_formats.size(); i++) {
      dest[i] = rt_formats[i];
    }
    return UINT(rt_formats.size());
  }

  DXGI_FORMAT get_dsv_format() const {
    return is_depth_stencil_null ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_D24_UNORM_S8_UINT;
  }
};

struct RayTracingShaderMetaDesc {
  RootSignatureDescMemory global {};
  std::wstring hitgroup_name;
  std::wstring closest_hit_name;
  // std::wstring any_hit_name;
  // std::wstring intersection_name;
  RootSignatureDescMemory hitgroup {};
  std::wstring raygen_name;
  RootSignatureDescMemory raygen {};
  std::wstring miss_name;
  RootSignatureDescMemory miss {};

  RayTracingShaderMetaDesc() {}

  RayTracingShaderMetaDesc(RaytracingShaderMetaInfo&& meta) {
    init(std::move(meta));
  }

  void init(RaytracingShaderMetaInfo&& meta) {
    hitgroup_name = std::move(meta.hitgroup_name);
    closest_hit_name = std::move(meta.closest_hit_name);
    raygen_name = std::move(meta.raygen_name);
    miss_name = std::move(meta.miss_name);
    global.init_signature(meta.global_signature, false);
    hitgroup.init_signature(meta.hitgroup_signature, true);
    raygen.init_signature(meta.raygen_signature, true);
    miss.init_signature(meta.miss_signature, true);
  }
 
private:
  // any form of copying is not allow
  RayTracingShaderMetaDesc(const RayTracingShaderMetaDesc&) = delete;
  RayTracingShaderMetaDesc(RayTracingShaderMetaDesc&&) = delete;
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
  FixedVec<D3D12_GPU_VIRTUAL_ADDRESS, 4> cbvs;

  D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor_gpu = {0};
  D3D12_CPU_DESCRIPTOR_HANDLE base_descriptor_cpu = {0};
  UINT alloc_index = -1;
  UINT alloc_num = -1;
};

struct RasterizationShaderData : ShaderData {
  using ShaderData::ShaderData;
  RasterizationShaderMetaDesc meta;
  ShaderCompileResult compiled_data;
};

struct RaytracingShaderData : ShaderData {
  using ShaderData::ShaderData;
  RayTracingShaderMetaDesc meta;

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

struct ModelData : BaseModelData {
  using BaseModelData::BaseModelData;
  // Bound aabb;
  std::shared_ptr<GeometryData> geometry;
  UINT const_buffer_index = UINT_MAX; // in shader cb buffer
  UINT cbv_index = UINT_MAX;          // in device shared heap

  UINT tlas_instance_id = UINT_MAX;

  void update_transform(Model& model) {
    // NOTE: check how ViewportData store the transforms
    REI_ASSERT(is_right_handed);
    this->transform = model.get_transform();
  }
};

} // namespace d3d

} // namespace rei

#endif

#endif
