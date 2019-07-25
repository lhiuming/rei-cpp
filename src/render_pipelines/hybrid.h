#ifndef REI_RENDER_PIPELINE_HYBRID_H
#define REI_RENDER_PIPELINE_HYBRID_H

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

public:
  HybridPipeline(RendererPtr renderer);

  virtual ViewportHandle register_viewport(ViewportConfig conf) override;
  virtual void remove_viewport(ViewportHandle viewport) override {}
  virtual void transform_viewport(ViewportHandle handle, const Camera& camera) override;

  virtual SceneHandle register_scene(SceneConfig conf) override;
  virtual void remove_scene(SceneHandle scene) override {}

  virtual void update_model(SceneHandle scene, const Model& model) override;
  virtual void add_model(SceneHandle scene, ModelHandle model) override {}

  virtual void render(ViewportHandle viewport, SceneHandle scene) override;

private:

  ShaderHandle m_gpass_shader;
  ShaderHandle m_raytracing_shader;
  ShaderHandle m_blit_shader;

  BufferHandle m_per_render_buffer;

  using RaytracingArgumentKey = std::pair<ViewportHandle, SceneHandle>;
  struct RakHash {
    // NOTE stole from boost
    // ref: https://www.boost.org/doc/libs/1_35_0/doc/html/boost/hash_combine_id241013.html
    void combine(size_t& seed, size_t v) const {
      seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    size_t operator()(const RaytracingArgumentKey& key) const { 
      size_t seed = 0;
      combine(seed, std::hash_value(key.first));
      combine(seed, std::hash_value(key.second));
      return seed;
    }
  };
  Hashmap<RaytracingArgumentKey, ShaderArgumentHandle, RakHash> m_raytracing_args;

  ShaderArgumentHandle fetch_raytracing_arg(ViewportHandle viewport, SceneHandle scene);
};

} // namespace rei

#endif