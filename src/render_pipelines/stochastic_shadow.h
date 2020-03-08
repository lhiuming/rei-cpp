#pragma once

#include "render_pass.h"
#include "renderer.h"
#include "renderer/graphics_handle.h"

// temporarily, for StochasticShadow::Parameters
#include <functional>

namespace rei {

class StochasticShadowPass : public RenderPass {
public:
  struct Parameters {
    size_t frame_id;
    int width;
    int height;
    int ssp_per_light;
    int lightCount;
    int* area_light_indices;
    Color* area_light_colors;
    Vec4* area_light_shapes;
    std::function<ShaderArgumentHandle(int)> area_light_arg_getter; 
    BufferHandle area_light_cb;
    BufferHandle unshadowed;
    BufferHandle depth_stencil_buffer;
    BufferHandle gbuffer0;
    BufferHandle gbuffer1;
    BufferHandle gbuffer2;
    BufferHandle per_render_buffer;
    BufferHandle tlas;
    BufferHandle shading_output;
  };

public:
  StochasticShadowPass(std::weak_ptr<Renderer> renderer);

  void run(const Parameters& params);

private:
  std::weak_ptr<Renderer> m_renderer;

  // Shaders
  ShaderHandle m_stochastic_shadow_sample_gen_shader;
  ShaderHandle m_stochastic_shadow_trace_shader;
  BufferHandle m_stochastic_shadow_trace_shadertable;
  ShaderHandle m_stochastic_shadow_denoise_firstpass_shader;
  ShaderHandle m_stochastic_shadow_denoise_finalpass_shader;

  // Buffers and arguments
  BufferHandle stochastic_sample_ray;
  BufferHandle stochastic_sample_radiance;
  BufferHandle stochastic_unshadowed;
  BufferHandle stochastic_shadowed;
  // BufferHandle stochastic_variance;
  BufferHandle denoised_unshadowed;
  BufferHandle denoised_shadowed;

  ShaderArgumentHandle sample_gen_pass_arg;
  // ShaderArgumentHandle variance_pass_arg;
  // ShaderArgumentHandle variance_filter_pass_arg;
  ShaderArgumentHandle denoise_fistpass_arg0;
  ShaderArgumentHandle denoise_finalpass_arg0;
  ShaderArgumentHandle denoise_common_arg1;

  ShaderArgumentHandle rt_shadow_pass_arg;

  bool buffer_create = false;

#if 0
  // debug pass
  ShaderArgumentHandle blit_stochastic_sample_ray;
  ShaderArgumentHandle blit_stochastic_sample_radiance;
  ShaderArgumentHandle blit_stochastic_unshadowed;
  ShaderArgumentHandle blit_stochastic_shadowed;
  ShaderArgumentHandle blit_denoised_unshadowed;
  ShaderArgumentHandle blit_denoised_shadowed;
#endif
};

} // namespace rei
