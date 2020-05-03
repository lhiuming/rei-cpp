#include "stochastic_shadow.h"

#include "editor/imgui_global.h"
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

StochasticShadowPass::StochasticShadowPass(std::weak_ptr<Renderer> renderer)
    : m_renderer(renderer), m_enabled(true) {
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
}

void StochasticShadowPass::run(const Parameters& params) {
  const int width = params.width;
  const int height = params.height;

  if (!enabled()) return;

  // Allocate Resources
  if (!buffer_create) {
    // TODO handle viewport size-changed
    buffer_create = true;

    auto r = m_renderer.lock();

    // area light resources
    stochastic_unshadowed = r->create_texture_2d(
      TextureDesc::unorder_access(width, height, ResourceFormat::R32G32B32A32Float),
      ResourceState::UnorderedAccess, L"Area Light Stochastic Unshadowed");
    stochastic_sample_ray = r->create_texture_2d(
      TextureDesc::unorder_access(width, height, ResourceFormat::R32G32B32A32Float),
      ResourceState::UnorderedAccess, L"Area Light Stochastic Sample-Ray");
    stochastic_sample_radiance = r->create_texture_2d(
      TextureDesc::unorder_access(width, height, ResourceFormat::R32G32B32A32Float),
      ResourceState::UnorderedAccess, L"Area Light Stochastic Sample-Radiance");
    stochastic_shadowed = r->create_texture_2d(
      TextureDesc::unorder_access(width, height, ResourceFormat::R32G32B32A32Float),
      ResourceState::UnorderedAccess, L"Area Light Stochastic Shadowed");
    denoised_shadowed = r->create_texture_2d(
      TextureDesc::unorder_access(width, height, ResourceFormat::R32G32B32A32Float),
      ResourceState::UnorderedAccess, L"Area Light Denoised Shadowed");
    denoised_unshadowed = r->create_texture_2d(
      TextureDesc::unorder_access(width, height, ResourceFormat::R32G32B32A32Float),
      ResourceState::UnorderedAccess, L"Area Light Denoised Unshadowed");

    {
      ShaderArgumentValue val {};
      val.shader_resources = {
        // G-buffers
        params.depth_stencil_buffer, // for position
        params.gbuffer0,             // albedo, metallic
        params.gbuffer1,             // normal
        params.gbuffer2              // emission
      };
      val.unordered_accesses
        = {stochastic_unshadowed, stochastic_sample_ray, stochastic_sample_radiance};
      val.const_buffers = {params.per_render_buffer};
      val.const_buffer_offsets
        = {0}; // FIXME current all viewport using the same part of per-render buffer
      sample_gen_pass_arg = r->create_shader_argument(val);
      REI_ASSERT(sample_gen_pass_arg);
    }
    {
      ShaderArgumentValue val1 {};
      val1.const_buffers = {params.per_render_buffer};
      val1.const_buffer_offsets = {0};
      val1.shader_resources = {params.depth_stencil_buffer, params.gbuffer0, params.unshadowed};
      denoise_common_arg1 = r->create_shader_argument(val1);
      ShaderArgumentValue first0 {};
      first0.shader_resources = {stochastic_shadowed, stochastic_unshadowed};
      first0.unordered_accesses = {denoised_shadowed, denoised_unshadowed};
      denoise_fistpass_arg0 = r->create_shader_argument(first0);
      ShaderArgumentValue final0 = first0;
      final0.unordered_accesses = {params.shading_output};
      denoise_finalpass_arg0 = r->create_shader_argument(final0);
    }
    // raytracing arguments
    {
      // Raytracing shadow pass argument
      ShaderArgumentValue val {};
      val.const_buffers = {params.per_render_buffer};
      val.const_buffer_offsets = {0};
      val.shader_resources = {
        params.tlas,                 // t0 TLAS
        params.depth_stencil_buffer, // t1 G-buffer depth
        stochastic_sample_ray,       // t2 sample ray
        stochastic_sample_radiance,  // t3 sample radiance
      };
      val.unordered_accesses = {stochastic_shadowed};
      rt_shadow_pass_arg = r->create_shader_argument(val);
      REI_ASSERT(rt_shadow_pass_arg);
    }
  }

  auto& cmd_list = m_renderer.lock();

  cmd_list->transition(stochastic_unshadowed, ResourceState::UnorderedAccess);
  cmd_list->transition(stochastic_shadowed, ResourceState::UnorderedAccess);
  cmd_list->clear_texture(stochastic_unshadowed, {0, 0, 0, 0}, ClearArea::all());
  cmd_list->clear_texture(stochastic_shadowed, {0, 0, 0, 0}, ClearArea::all());
  cmd_list->barrier(stochastic_unshadowed);
  cmd_list->barrier(stochastic_shadowed);

  // generate stochastic sample, and trace
  // NOTE: per light, multiple ray-per-pixel
  HaltonSequence halton {params.frame_id};
  for (int i = 0; i < params.lightCount; i++) {
    int light_index = params.area_light_indices[i];
    Vec4 light_color = Vec4(params.area_light_colors[i]);
    Vec4 light_shape = params.area_light_shapes[i];

    // update light data and random data
    cmd_list->update_const_buffer(params.area_light_cb, light_index, 0, light_shape);
    cmd_list->update_const_buffer(params.area_light_cb, light_index, 1, light_color);

    for (int sp = 0; sp < params.ssp_per_light; sp++) {
      // update random data for each sample pass
      cmd_list->update_const_buffer(params.area_light_cb, light_index, 2, halton.next4());

      // sample gen
      cmd_list->transition(stochastic_sample_ray, ResourceState::UnorderedAccess);
      cmd_list->transition(stochastic_sample_radiance, ResourceState::UnorderedAccess);
      cmd_list->clear_texture(stochastic_sample_ray, {0, 0, 0, 0}, ClearArea::all());
      cmd_list->clear_texture(stochastic_sample_radiance, {0, 0, 0, 0}, ClearArea::all());
      cmd_list->barrier(stochastic_sample_ray);
      cmd_list->barrier(stochastic_sample_radiance);
      DispatchCommand dispatch {};
      dispatch.compute_shader = m_stochastic_shadow_sample_gen_shader;
      dispatch.arguments = {params.area_light_arg_getter(light_index), sample_gen_pass_arg};
      dispatch.dispatch_x = width / 8;
      dispatch.dispatch_y = height / 8;
      dispatch.dispatch_z = 1;
      cmd_list->dispatch(dispatch);

      // shadow trace
      cmd_list->transition(stochastic_sample_ray, ResourceState::ComputeShaderResource);
      cmd_list->transition(stochastic_sample_radiance, ResourceState::ComputeShaderResource);
      cmd_list->barrier(stochastic_sample_ray);
      cmd_list->barrier(stochastic_sample_radiance);
      RaytraceCommand trace {};
      trace.raytrace_shader = m_stochastic_shadow_trace_shader;
      trace.arguments = {rt_shadow_pass_arg};
      trace.shader_table = m_stochastic_shadow_trace_shadertable;
      trace.width = width;
      trace.height = height;
      cmd_list->raytrace(trace);
    } // end for each ssp

  } // end for each area light

  // Two-pass denoise -> output final shade //
  cmd_list->transition(stochastic_shadowed, ResourceState::ComputeShaderResource);
  cmd_list->transition(stochastic_unshadowed, ResourceState::ComputeShaderResource);
  cmd_list->transition(denoised_shadowed, ResourceState::UnorderedAccess);
  cmd_list->transition(denoised_unshadowed, ResourceState::UnorderedAccess);
  cmd_list->clear_texture(denoised_shadowed, Vec4(0, 0, 0, 0), ClearArea::all());
  cmd_list->clear_texture(denoised_unshadowed, Vec4(0, 0, 0, 0), ClearArea::all());
  cmd_list->barrier(denoised_shadowed);
  cmd_list->barrier(denoised_unshadowed);
  {
    DispatchCommand dispatch {};
    dispatch.compute_shader = m_stochastic_shadow_denoise_firstpass_shader;
    dispatch.arguments = {denoise_fistpass_arg0, denoise_common_arg1};
    dispatch.dispatch_x = width / 8;
    dispatch.dispatch_y = height / 8;
    dispatch.dispatch_z = 1;
    cmd_list->dispatch(dispatch);
  }
  cmd_list->transition(denoised_shadowed, ResourceState::ComputeShaderResource);
  cmd_list->transition(denoised_unshadowed, ResourceState::ComputeShaderResource);
  {
    DispatchCommand dispatch {};
    dispatch.compute_shader = m_stochastic_shadow_denoise_finalpass_shader;
    dispatch.arguments = {denoise_finalpass_arg0, denoise_common_arg1};
    dispatch.dispatch_x = width / 8;
    dispatch.dispatch_y = height / 8;
    dispatch.dispatch_z = 1;
    cmd_list->dispatch(dispatch);
  }
}

void StochasticShadowPass::on_gui() {
  if (g_ImGUI.show_collapsing_header("Stocastic Shadow Pass")) {
    g_ImGUI.show_checkbox("Enabled", &this->m_enabled);
  }
}

} // namespace rei
