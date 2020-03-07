#pragma once

#include "renderer/graphics_handle.h"
#include "renderer.h"
#include "render_pass.h"

namespace rei {

class StochasticShadow : public RenderPass {
public:
  StochasticShadow(std::weak_ptr<Renderer> renderer);

  void run(BufferHandle unshadowed, size_t frame_id, int m_area_shadow_ssp_per_light);

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

  ShaderArgumentHandle unshadowed_pass_arg;
  ShaderArgumentHandle sample_gen_pass_arg;
  // ShaderArgumentHandle variance_pass_arg;
  // ShaderArgumentHandle variance_filter_pass_arg;
  ShaderArgumentHandle denoise_fistpass_arg0;
  ShaderArgumentHandle denoise_finalpass_arg0;
  ShaderArgumentHandle denoise_common_arg1;

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
