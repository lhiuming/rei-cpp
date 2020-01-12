#include "d3d_common_resources.h"

#include "../renderer.h"

#include <typeinfo>

namespace rei {

namespace d3d {

RenderTargetSpec::RenderTargetSpec() {
  sample_desc = DXGI_SAMPLE_DESC {1, 0};
  rt_format = ResourceFormat::B8G8R8A8_UNORM;
  ds_format = ResourceFormat::D24_UNORM_S8_UINT;
  dxgi_rt_format = DXGI_FORMAT_R8G8B8A8_UNORM;
  dxgi_ds_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  REI_ASSERT(is_right_handed);
  ds_clear.Depth = 0.0;
  ds_clear.Stencil = 0;
}

const D3D12_INPUT_ELEMENT_DESC c_input_layout[3]
  = {{
       "POSITION", 0,                  // a Name and an Index to map elements in the shader
       DXGI_FORMAT_R32G32B32A32_FLOAT, // enum member of DXGI_FORMAT; define the format of the
                                       // element
       0,                              // input slot; kind of a flexible and optional configuration
       0,                              // byte offset
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // ADVANCED, discussed later; about instancing
       0                                           // ADVANCED; also for instancing
     },
    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
      sizeof(VertexElement::pos), // skip the first 3 coordinate data
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
      sizeof(VertexElement::pos)
        + sizeof(VertexElement::color), // skip the fisrt 3 coordinnate and 4 colors ata
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

ID3D12Resource* BufferData::get_res() {
  return res.match( //
    [](const TextureBuffer& tex) { return tex.buffer.Get(); },
    [](const BlasBuffer& blas) { return blas.buffer.Get(); },
    [](const auto& b) {
      REI_NOT_IMPLEMENT();
      const std::type_info& i = typeid(b);
      warning(i.name());
      return (ID3D12Resource*)NULL;
    });
}

void RootSignatureDescMemory::init_signature(const ShaderSignature& signature, bool local) {
    auto& param_table = signature.param_table;

    range_memory.clear();
    param_memory.clear();

    for (int space = 0; space < param_table.size(); space++) {
      auto& params = param_table[space];
      size_t range_offset = range_memory.size();

      // CBV/SRV/UAVs
      if (params.const_buffers.size())
        range_memory.emplace_back(
          D3D12_DESCRIPTOR_RANGE_TYPE_CBV, params.const_buffers.size(), 0, space);
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
        range_memory.emplace_back(
          D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, params.samplers.size(), 0, space);

      UINT new_range_count = UINT(range_memory.size() - range_offset);
      if (new_range_count > 0) {
        param_memory.emplace_back().InitAsDescriptorTable(
          new_range_count, &range_memory[range_offset]);
      }

      // Static Sampler
      for (int i = 0; i < params.static_samplers.size(); i++) {
        // auto sampler = params.static_samplers[i];
        auto& sampler_desc = static_sampler_memory.emplace_back();
        sampler_desc.Init(i, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT);
        sampler_desc.RegisterSpace = space;
      }
    }

    D3D12_ROOT_SIGNATURE_FLAGS flags
      = local ? D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE : D3D12_ROOT_SIGNATURE_FLAG_NONE;
    // NOTE: awaly init the desc
    //if (param_memory.size() > 0 || static_sampler_memory.size() > 0)
    desc.Init(param_memory.size(), param_memory.data(), static_sampler_memory.size(),
      static_sampler_memory.data(), flags);
  }
 

RasterShaderDesc::RasterShaderDesc(RasterizationShaderMetaInfo&& meta) {
    this->init(std::move(meta));
  }

  void RasterShaderDesc::init(RasterizationShaderMetaInfo&& meta) {
    root_signature.init_signature(meta.signature, false);
    root_signature.desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    for (auto& rt_desc : meta.render_target_descs) {
      rt_formats.push_back(to_dxgi_format(rt_desc.format));
    }
    is_depth_stencil_null = meta.is_depth_stencil_disabled;
    if (meta.is_blending_addictive) {
      blend_state.RenderTarget[0].BlendEnable = true;
      blend_state.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
      blend_state.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
      blend_state.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    }
  }


  void ComputeShaderDesc::init(ComputeShaderMetaInfo&& meta) { signature.init_signature(meta.signature, false); }

  RayTraceShaderDesc::RayTraceShaderDesc(RaytracingShaderMetaInfo&& meta) {
    init(std::move(meta));
  }

  void RayTraceShaderDesc::init(RaytracingShaderMetaInfo&& meta) {
    hitgroup_name = std::move(meta.hitgroup_name);
    closest_hit_name = std::move(meta.closest_hit_name);
    raygen_name = std::move(meta.raygen_name);
    miss_name = std::move(meta.miss_name);
    global.init_signature(meta.global_signature, false);
    hitgroup.init_signature(meta.hitgroup_signature, true);
    raygen.init_signature(meta.raygen_signature, true);
    miss.init_signature(meta.miss_signature, true);
  }


  } // namespace d3d

} // namespace rei
