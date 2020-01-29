#pragma once

#include "d3dx12.h"

#include "debug.h"
#include "container_utils.h"

#include "graphics_const.h"

namespace rei {

//-----------------------------------------------------------------------------
// Forward decls

// shaders 
struct RasterizationShaderMetaInfo;
struct ComputeShaderMetaInfo;
struct RaytracingShaderMetaInfo;

//-----------------------------------------------------------------------------
// Enums

enum class ResourceFormat {
  Undefined,
  R32G32B32A32Float,
  R32G32Float,
  R8G8B8A8Unorm,
  D24Unorm_S8Uint,
  AcclerationStructure,
  Count
};

enum class ResourceDimension {
  Undefined,
  Raw,
  StructuredBuffer,
  Texture1D,
  Texture1DArray,
  Texture2D,
  Texture2DArray,
  Texture3D,
  AccelerationStructure,
};

enum class ResourceState {
  Undefined,
  Present,
  RenderTarget,
  DeptpWrite,
  CopySource,
  CopyDestination,
  PixelShaderResource,
  ComputeShaderResource,
  UnorderedAccess,
};


//-----------------------------------------------------------------------------
// Descroption structs

struct RenderTargetDesc {
  ResourceFormat format = ResourceFormat::R8G8B8A8Unorm;
};

// TODO allow more customized configuration
struct MergeDesc {
  bool is_blending_addictive = false;
  bool is_alpha_blending = false;
  void init_as_addictive() {
    is_blending_addictive = true;
    is_alpha_blending = false;
  }
  void init_as_alpha_blending() {
    is_blending_addictive = false;
    is_alpha_blending = true;
  }
};

struct VertexInputDesc {
  std::string semantic;
  unsigned char sementic_index;
  ResourceFormat format;
  unsigned char byte_offset;
};

constexpr UINT c_input_layout_num = 3;
extern const D3D12_INPUT_ELEMENT_DESC c_input_layout[3];

// Pramater types
// NOTE: each represent a range type in descriptor table
struct ConstantBuffer {};
struct ShaderResource {};
struct UnorderedAccess {};
struct Sampler {};
struct StaticSampler {};

// Represent a descriptor table / descriptor set, in a defined space
struct ShaderParameter {
  // TODO make this inplace memory
  // index as implicit shader register
  FixedVec<ConstantBuffer, 8> const_buffers;
  FixedVec<ShaderResource, 8> shader_resources;
  FixedVec<UnorderedAccess, 8> unordered_accesses;
  FixedVec<Sampler, 8> samplers;
  FixedVec<StaticSampler, 8> static_samplers;
};

// Represent the set of shader resources to be bound by a list of shader arguments
struct ShaderSignature {
  // index as implicit register space
  // NOTE: you dont usually use more than 4 space, right?
  FixedVec<ShaderParameter, 4> param_table;
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
  void init_signature(const ShaderSignature& signature, bool local);
};

struct VertexInputLayoutMemory {
  using MetaInput = FixedVec<VertexInputDesc, 8>;

  FixedVec<std::string, MetaInput::max_size> semantic_names;
  FixedVec<D3D12_INPUT_ELEMENT_DESC, MetaInput::max_size> descs;

  void init(MetaInput&& metas);

  D3D12_INPUT_LAYOUT_DESC get_layout_desc() const {
    if (descs.size() == 0) { return {c_input_layout, c_input_layout_num}; }
    return {descs.data(), UINT(descs.size())};
  }
};

struct RasterShaderDesc {
  CD3DX12_RASTERIZER_DESC raster_state = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  CD3DX12_DEPTH_STENCIL_DESC depth_stencil = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  bool is_depth_stencil_null = false;
  CD3DX12_BLEND_DESC blend_state = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

  CD3DX12_STATIC_SAMPLER_DESC static_sampler_desc {};
  RootSignatureDescMemory root_signature {};

  VertexInputLayoutMemory input_layout {};

  FixedVec<DXGI_FORMAT, 8> rt_formats;

  RasterShaderDesc() {
    REI_ASSERT(is_right_handed);
    // we use right-hand coordiante throughout the pipeline
    depth_stencil.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
  }

  RasterShaderDesc(RasterizationShaderMetaInfo&& meta);

  void init(RasterizationShaderMetaInfo&& meta);

  UINT get_rtv_formats(DXGI_FORMAT (&dest)[8]) const {
    for (int i = 0; i < rt_formats.size(); i++) {
      dest[i] = rt_formats[i];
    }
    for (int i = rt_formats.size(); i < 8; i++) {
      dest[i] = DXGI_FORMAT_UNKNOWN;
    }
    return UINT(rt_formats.size());
  }

  DXGI_FORMAT get_dsv_format() const {
    return is_depth_stencil_null ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_D24_UNORM_S8_UINT;
  }
};

struct ComputeShaderDesc {
  RootSignatureDescMemory signature {};

  ComputeShaderDesc() {}
  void init(ComputeShaderMetaInfo&& meta);
};

struct RayTraceShaderDesc {
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

  RayTraceShaderDesc() {}

  RayTraceShaderDesc(RaytracingShaderMetaInfo&& meta);

  void init(RaytracingShaderMetaInfo&& meta);

private:
  // any form of copying is not allow
  RayTraceShaderDesc(const RayTraceShaderDesc&) = delete;
  RayTraceShaderDesc(RayTraceShaderDesc&&) = delete;
};

} // namespace rei

