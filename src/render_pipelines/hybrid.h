#ifndef REI_RENDER_PIPELINE_HYBRID_H
#define REI_RENDER_PIPELINE_HYBRID_H

#include <functional>
#include <string>

#include "render_pipeline_base.h"

namespace rei {

// FIXME lazyness
namespace d3d {
class Renderer;
}

// Forwared decl
namespace hybrid {
struct ViewportProxy;
struct SceneProxy;
} // namespace hybrid

//
// Hybrid pipeline mixing deferred-rasterizing and raytracing
//
class HybridPipeline
    : public SimplexPipeline<hybrid::ViewportProxy, hybrid::SceneProxy, d3d::Renderer> {
  // FIXME lazyness
  using Renderer = d3d::Renderer;
  using ViewportProxy = hybrid::ViewportProxy;
  using SceneProxy = hybrid::SceneProxy;

public:
  HybridPipeline(RendererPtr renderer);

  ViewportHandle register_viewport(ViewportConfig conf) override;
  void remove_viewport(ViewportHandle viewport) override {}
  void transform_viewport(ViewportHandle handle, const Camera& camera) override;

  SceneHandle register_scene(SceneConfig conf) override;
  void remove_scene(SceneHandle scene) override {}

  void add_model(SceneHandle scene, const Model& model, Scene::ModelUID model_id) override {}
  void update_model(SceneHandle scene, const Model& model, Scene::ModelUID model_id) override;

  virtual void render(ViewportHandle viewport, SceneHandle scene) override;

private:
  const bool m_enable_jittering = true;
  const bool m_enable_multibounce = true;
  const bool m_enabled_accumulated_rtrt = true;
  const int m_area_shadow_ssp_per_light = 4;

  ShaderHandle m_gpass_shader;

  ShaderHandle m_base_shading_shader;
  ShaderHandle m_punctual_lighting_shader;
  ShaderHandle m_area_lighting_shader;

  ShaderHandle m_stochastic_shadow_sample_gen_shader;
  ShaderHandle m_stochastic_shadow_trace_shader;
  BufferHandle m_stochastic_shadow_trace_shadertable;
  //ShaderHandle m_stochastic_shadow_denoise_horizontal_shader;
  //ShaderHandle m_stochastic_shadow_denoise_final_shader;

  ShaderHandle m_multibounce_shader;

  ShaderHandle m_taa_shader;
  ShaderHandle m_blit_shader;

  BufferHandle m_per_render_buffer;

  using CombinedArgumentKey = std::pair<ViewportHandle, SceneHandle>;
  struct RakHash {
    // NOTE stole from boost
    // ref: https://www.boost.org/doc/libs/1_35_0/doc/html/boost/hash_combine_id241013.html
    void combine(size_t& seed, size_t v) const {
      seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    size_t operator()(const CombinedArgumentKey& key) const {
      size_t seed = 0;
      combine(seed, std::hash_value(key.first));
      combine(seed, std::hash_value(key.second));
      return seed;
    }
  };
  Hashmap<CombinedArgumentKey, ShaderArgumentHandle, RakHash> m_raytracing_args;
  Hashmap<CombinedArgumentKey, ShaderArgumentHandle, RakHash> m_shadow_tracing_args;

  ShaderArgumentHandle fetch_raytracing_arg(ViewportHandle viewport, SceneHandle scene);
  ShaderArgumentHandle fetch_shadow_tracing_arg(ViewportHandle viewport, SceneHandle scene);

  ShaderArgumentHandle fetch_direct_punctual_lighting_arg(SceneProxy& scene, int cb_index);
  ShaderArgumentHandle fetch_direct_area_lighting_arg(SceneProxy& scene, int cb_index);
};

} // namespace rei

#endif