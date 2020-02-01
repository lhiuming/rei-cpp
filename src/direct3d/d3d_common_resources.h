#ifndef REI_D3D_COMMON_RESOURCES_H
#define REI_D3D_COMMON_RESOURCES_H

#include <array>
#include <memory>

#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_4.h>
#include <windows.h>
#include <wrl.h>

#include "common.h"
//#include "../scene.h"
//#include "../algebra.h"
//#include "../camera.h"
//#include "../color.h"
//#include "../shader_struct.h"

#include "d3d_utils.h"

#include "renderer/graphics_handle.h"
#include "renderer/graphics_desc.h"

namespace rei {

class ShaderSignature;
class RasterizationShaderMetaInfo;
class ComputeShaderMetaInfo;
class RaytracingShaderMetaInfo;

namespace d3d {

using Microsoft::WRL::ComPtr;
using std::move;

// Some contants
constexpr DXGI_FORMAT c_index_format = DXGI_FORMAT_R16_UINT;
constexpr DXGI_FORMAT c_accel_struct_vertex_pos_format = DXGI_FORMAT_R32G32B32_FLOAT;


inline static constexpr UINT64 get_bytesize(DXGI_FORMAT format) {
  switch (format) {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      return 4;
    default:
      REI_ERROR("Unhandeled format");
      return 1;
  }
}

inline static constexpr DXGI_FORMAT to_dxgi_format(ResourceFormat format) {
  switch (format) {
    case ResourceFormat::R32G32B32A32Float:
      return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case ResourceFormat::R32G32Float:
      return DXGI_FORMAT_R32G32_FLOAT;
    case ResourceFormat::R8G8B8A8Unorm:
      return DXGI_FORMAT_R8G8B8A8_UNORM;
    case ResourceFormat::D24Unorm_S8Uint:
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
  if (format == ResourceFormat::D24Unorm_S8Uint) { return DXGI_FORMAT_R24_UNORM_X8_TYPELESS; }
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
    case ResourceState::Undefined:
      return D3D12_RESOURCE_STATE_COMMON;
    case ResourceState::Present:
      return D3D12_RESOURCE_STATE_PRESENT;
    case ResourceState::RenderTarget:
      return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case ResourceState::DeptpWrite:
      return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case ResourceState::CopySource:
      return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case ResourceState::CopyDestination:
      return D3D12_RESOURCE_STATE_COPY_DEST;
    case ResourceState::PixelShaderResource:
      return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case ResourceState::ComputeShaderResource:
      return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
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
  VertexElement(const Vec3& p, const Color& c, const Vec3& n)
      : pos(p.x, p.y, p.z, 1), color(c.r, c.g, c.b, c.a), normal(n.x, n.y, n.z) {}
  VertexElement(
    float x, float y, float z, float r, float g, float b, float a, float nx, float ny, float nz)
      : pos(x, y, z, 1), color(r, g, b, a), normal(nx, ny, nz) {}
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

struct SwapchainData : BaseSwapchainData {
  using BaseSwapchainData::BaseSwapchainData;
  // TODO deelete the viewport-resource class
  ComPtr<IDXGISwapChain3> res_object;
  FixedVec<BufferHandle, 8> render_targets;
  UINT curr_rt_index = 0;

  BufferHandle get_curr_render_target() const { return render_targets[curr_rt_index]; }
  void advance_curr_rt() { curr_rt_index = (curr_rt_index + 1) % render_targets.size(); }
};

struct MeshUploadResult {
  ComPtr<ID3D12Resource> vert_buffer;
  ComPtr<ID3D12Resource> vert_upload_buffer;
  UINT vertex_num = UINT_MAX;

  ComPtr<ID3D12Resource> ind_buffer;
  ComPtr<ID3D12Resource> ind_upload_buffer;
  UINT index_num = UINT_MAX;

  ComPtr<ID3D12Resource> blas_buffer;
  ComPtr<ID3D12Resource> scratch_buffer;
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
  RasterShaderDesc meta;
  ComPtr<ID3D12PipelineState> pso;
};

struct ComputeShaderData : ShaderData {
  using ShaderData::ShaderData;
  ComputeShaderDesc meta;
  ComPtr<ID3D12PipelineState> pso;
};

struct RaytracingShaderData : ShaderData {
  using ShaderData::ShaderData;
  RayTraceShaderDesc meta;

  struct LocalShaderData {
    ComPtr<ID3D12RootSignature> root_signature;
    const void* shader_id;
  };
  LocalShaderData raygen;
  LocalShaderData hitgroup;
  LocalShaderData miss;

  ComPtr<ID3D12StateObject> pso;
};

} // namespace d3d

} // namespace rei

#endif

