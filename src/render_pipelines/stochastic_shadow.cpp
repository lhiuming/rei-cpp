#include "stochastic_shadow.h"
#include "math/halton.h"

namespace rei {

struct HybridStochasticGenShaderDesc : ComputeShaderMetaInfo {
  HybridStochasticGenShaderDesc() {
    ShaderParameter space0 {};
    space0.const_buffers = {
      ConstantBuffer(), // light data
    };
    ShaderParameter space1 {};
    space1.shader_resources = {
      ShaderResource(), // depth
      ShaderResource(), // gbuffers
      ShaderResource(),
      ShaderResource(),
    };
    space1.unordered_accesses = {
      UnorderedAccess(), // output U_n
      UnorderedAccess(), // output ray sample
      UnorderedAccess(), // output radiance sample
    };
    space1.const_buffers = {
      ConstantBuffer(), // per-render data
    };
    signature.param_table = {space0, space1};
  }
};

struct HybridShtochasticShadowTraceShaderDesc : RaytracingShaderMetaInfo {
  HybridShtochasticShadowTraceShaderDesc() {
    ShaderParameter space0 {};
    space0.const_buffers = {ConstantBuffer()};       // Per-render CN
    space0.shader_resources = {ShaderResource(),     // TLAS
      ShaderResource(),                              // depth buffer
      ShaderResource(),                              // sample ray
      ShaderResource()};                             // sample radiance
    space0.unordered_accesses = {UnorderedAccess()}; // output buffer
    global_signature.param_table = {space0};

    hitgroup_name = L"hit_group0";
    raygen_name = L"raygen_shader";
    closest_hit_name = L"closest_hit_shader";
    miss_name = L"miss_shader";
  }
};

struct HybridStochasticShadowDenoiseShaderDesc : ComputeShaderMetaInfo {
  HybridStochasticShadowDenoiseShaderDesc(bool finalpass = false) {
    ShaderParameter space0 {};
    space0.shader_resources = {
      ShaderResource(), // two input
      ShaderResource(),
    };
    if (finalpass) {
      space0.unordered_accesses = {
        UnorderedAccess(), // sigle output
      };
    } else {
      space0.unordered_accesses = {
        UnorderedAccess(), // two output
        UnorderedAccess(),
      };
    }
    ShaderParameter space1 {};
    space1.shader_resources = {
      ShaderResource(), // depth
      ShaderResource(), // gbuffers - rt0 normal
      ShaderResource(), // unshadowd analytic radiance
      // ShaderResource(), // stochastic variance estimate
    };
    space1.const_buffers = {
      ConstantBuffer(), // per-render data
    };
    signature.param_table = {space0, space1};
  }
};

StochasticShadow::StochasticShadow(std::weak_ptr<Renderer> renderer) : m_renderer(renderer) {
#if SSAL
  auto r = renderer.lock();
  m_stochastic_shadow_sample_gen_shader
    = r->create_shader(L"CoreData/shader/hybrid_stochastic_shadow_sample_generate.hlsl",
      HybridStochasticGenShaderDesc());
  m_stochastic_shadow_trace_shader
    = r->create_shader(L"CoreData/shader/hybrid_stochastic_shadow_trace.hlsl",
      HybridShtochasticShadowTraceShaderDesc());
  m_stochastic_shadow_trace_shadertable
    = r->create_shader_table(1, m_stochastic_shadow_trace_shader);
  m_stochastic_shadow_denoise_firstpass_shader
    = r->create_shader(L"CoreData/shader/hybrid_stochastic_shadow_denoise.hlsl",
      HybridStochasticShadowDenoiseShaderDesc());
  m_stochastic_shadow_denoise_finalpass_shader
    = r->create_shader(L"CoreData/shader/hybrid_stochastic_shadow_denoise.hlsl",
      HybridStochasticShadowDenoiseShaderDesc(true),
      ShaderCompileConfig::defines<1>({{"FINAL_PASS", "1"}}));


    // area light resources
    proxy.area_light.stochastic_unshadowed = r->create_texture_2d(
      TextureDesc::unorder_access(width, height, ResourceFormat::R32G32B32A32Float),
      ResourceState::UnorderedAccess, L"Area Light Stochastic Unshadowed");
    proxy.area_light.stochastic_sample_ray = r->create_texture_2d(
      TextureDesc::unorder_access(width, height, ResourceFormat::R32G32B32A32Float),
      ResourceState::UnorderedAccess, L"Area Light Stochastic Sample-Ray");
    proxy.area_light.stochastic_sample_radiance = r->create_texture_2d(
      TextureDesc::unorder_access(width, height, ResourceFormat::R32G32B32A32Float),
      ResourceState::UnorderedAccess, L"Area Light Stochastic Sample-Radiance");
    proxy.area_light.stochastic_shadowed = r->create_texture_2d(
      TextureDesc::unorder_access(width, height, ResourceFormat::R32G32B32A32Float),
      ResourceState::UnorderedAccess, L"Area Light Stochastic Shadowed");
    proxy.area_light.denoised_shadowed = r->create_texture_2d(
      TextureDesc::unorder_access(width, height, ResourceFormat::R32G32B32A32Float),
      ResourceState::UnorderedAccess, L"Area Light Denoised Shadowed");
    proxy.area_light.denoised_unshadowed = r->create_texture_2d(
      TextureDesc::unorder_access(width, height, ResourceFormat::R32G32B32A32Float),
      ResourceState::UnorderedAccess, L"Area Light Denoised Unshadowed");



  {
    ShaderArgumentValue val {};
    val.shader_resources
      = {proxy.depth_stencil_buffer, proxy.gbuffer0, proxy.gbuffer1, proxy.gbuffer2};
    val.unordered_accesses = {proxy.area_light.stochastic_unshadowed,
      proxy.area_light.stochastic_sample_ray, proxy.area_light.stochastic_sample_radiance};
    val.const_buffers = {m_per_render_buffer};
    val.const_buffer_offsets
      = {0}; // FIXME current all viewport using the same part of per-render buffer
    proxy.area_light.sample_gen_pass_arg = r->create_shader_argument(val);
    REI_ASSERT(proxy.area_light.sample_gen_pass_arg);
  }
  {
    ShaderArgumentValue val1 {};
    val1.const_buffers = {m_per_render_buffer};
    val1.const_buffer_offsets = {0};
    val1.shader_resources
      = {proxy.depth_stencil_buffer, proxy.gbuffer0, proxy.area_light.unshadowed};
    proxy.area_light.denoise_common_arg1 = r->create_shader_argument(val1);
    ShaderArgumentValue first0 {};
    first0.shader_resources
      = {proxy.area_light.stochastic_shadowed, proxy.area_light.stochastic_unshadowed};
    first0.unordered_accesses
      = {proxy.area_light.denoised_shadowed, proxy.area_light.denoised_unshadowed};
    proxy.area_light.denoise_fistpass_arg0 = r->create_shader_argument(first0);
    ShaderArgumentValue final0 = first0;
    final0.unordered_accesses = {proxy.deferred_shading_output};
    proxy.area_light.denoise_finalpass_arg0 = r->create_shader_argument(final0);
  }

#endif
}

void StochasticShadow::run(BufferHandle unshadowed, size_t frame_id, int m_area_shadow_ssp_per_light) {
#if SSAL
  auto& cmd_list = m_renderer.lock();

  cmd_list->transition(unshadowed, ResourceState::UnorderedAccess);
  cmd_list->transition(stochastic_shadowed, ResourceState::UnorderedAccess);
  cmd_list->clear_texture(stochastic_unshadowed, {0, 0, 0, 0}, RenderArea::full());
  cmd_list->clear_texture(stochastic_shadowed, {0, 0, 0, 0}, RenderArea::full());
  cmd_list->barrier(stochastic_unshadowed);
  cmd_list->barrier(stochastic_shadowed);

  // generate stochastic sample, and trace
  // NOTE: per light, multiple ray-per-pixel
  HaltonSequence halton {frame_id};
  for (int i = 0; i < area_light_indices.size(); i++) {
    int light_index = area_light_indices[i];
    Vec4 light_color = Vec4(area_light_colors[i]);
    Vec4 light_shape = area_light_shapes[i];

    // update light data and random data
    cmd_list->update_const_buffer(scene.area_lights, light_index, 0, light_shape);
    cmd_list->update_const_buffer(scene.area_lights, light_index, 1, light_color);

    for (int sp = 0; sp < m_area_shadow_ssp_per_light; sp++) {
      // update random data for each sample pass
      cmd_list->update_const_buffer(scene.area_lights, light_index, 2, halton.next4());

      // sample gen
      cmd_list->transition(stochastic_sample_ray, ResourceState::UnorderedAccess);
      cmd_list->transition(stochastic_sample_radiance, ResourceState::UnorderedAccess);
      cmd_list->clear_texture(stochastic_sample_ray, {0, 0, 0, 0}, RenderArea::full());
      cmd_list->clear_texture(stochastic_sample_radiance, {0, 0, 0, 0}, RenderArea::full());
      cmd_list->barrier(stochastic_sample_ray);
      cmd_list->barrier(stochastic_sample_radiance);
      DispatchCommand dispatch {};
      dispatch.compute_shader = m_stochastic_shadow_sample_gen_shader;
      dispatch.arguments
        = {fetch_direct_area_lighting_arg(renderer, scene, light_index), sample_gen_pass_arg};
      dispatch.dispatch_x = viewport.width / 8;
      dispatch.dispatch_y = viewport.height / 8;
      dispatch.dispatch_z = 1;
      cmd_list->dispatch(dispatch);

      // shadow trace
      cmd_list->transition(
        viewport.area_light.stochastic_sample_ray, ResourceState::ComputeShaderResource);
      cmd_list->transition(
        viewport.area_light.stochastic_sample_radiance, ResourceState::ComputeShaderResource);
      cmd_list->barrier(viewport.area_light.stochastic_sample_ray);
      cmd_list->barrier(viewport.area_light.stochastic_sample_radiance);
      RaytraceCommand trace {};
      trace.raytrace_shader = m_stochastic_shadow_trace_shader;
      trace.arguments = {hybrid.rt_shdow_pass_arg};
      trace.shader_table = m_stochastic_shadow_trace_shadertable;
      trace.width = viewport.width;
      trace.height = viewport.height;
      cmd_list->raytrace(trace);
    } // end for each ssp

  } // end for each area light

  // Two-pass denoise -> output final shade //
  cmd_list->transition(
    stochastic_shadowed, ResourceState::ComputeShaderResource);
  cmd_list->transition(
    stochastic_unshadowed, ResourceState::ComputeShaderResource);
  cmd_list->transition(denoised_shadowed, ResourceState::UnorderedAccess);
  cmd_list->transition(denoised_unshadowed, ResourceState::UnorderedAccess);
  cmd_list->clear_texture(denoised_shadowed, Vec4(0, 0, 0, 0), RenderArea::full());
  cmd_list->clear_texture(denoised_unshadowed, Vec4(0, 0, 0, 0), RenderArea::full());
  cmd_list->barrier(denoised_shadowed);
  cmd_list->barrier(denoised_unshadowed);
  {
    DispatchCommand dispatch {};
    dispatch.compute_shader = m_stochastic_shadow_denoise_firstpass_shader;
    dispatch.arguments = {denoise_fistpass_arg0, denoise_common_arg1};
    dispatch.dispatch_x = viewport.width / 8;
    dispatch.dispatch_y = viewport.height / 8;
    dispatch.dispatch_z = 1;
    cmd_list->dispatch(dispatch);
  }
  cmd_list->transition(denoised_shadowed, ResourceState::ComputeShaderResource);
  cmd_list->transition(denoised_unshadowed, ResourceState::ComputeShaderResource);
  {
    DispatchCommand dispatch {};
    dispatch.compute_shader = m_stochastic_shadow_denoise_finalpass_shader;
    dispatch.arguments = {denoise_finalpass_arg0, denoise_common_arg1};
    dispatch.dispatch_x = viewport.width / 8;
    dispatch.dispatch_y = viewport.height / 8;
    dispatch.dispatch_z = 1;
    cmd_list->dispatch(dispatch);
  }
#endif
}

}
