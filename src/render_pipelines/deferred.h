#ifndef REI_DEFERRED_H
#define REI_DEFERRED_H

#include "render_pipeline_base.h"

namespace rei {

namespace d3d {
class Renderer;
}

namespace deferred {
struct ViewportData {
  SwapchainHandle swapchain;
};
struct SceneData {};
} // namespace deferred

class DeferredPipeline
    : public SimplexPipeline<deferred::ViewportData, deferred::SceneData, d3d::Renderer> {
public:
  DeferredPipeline(RendererPtr renderer);

  virtual ViewportHandle register_viewport(ViewportConfig conf) override;
  virtual void remove_viewport(ViewportHandle viewport) override {}

  virtual SceneHandle register_scene(SceneConfig conf) override;
  virtual void remove_scene(SceneHandle scene) override {}
  virtual void add_model(SceneHandle scene, ModelHandle model) override {}

  virtual void transform_viewport(ViewportHandle handle, const Camera& camera) override;

  virtual void render(ViewportHandle viewport, SceneHandle scene) override;

private:

};

} // namespace rei

#endif