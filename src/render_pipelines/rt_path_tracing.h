#ifndef REI_RT_PATH_TRACING_H
#define REI_RT_PATH_TRACING_H

#include <memory>
#include <vector>
#include <unordered_map>

#include "../camera.h"
#include "../render_pipeline.h";
// TODO for laziness
#include "../direct3d/d3d_renderer.h";

namespace rei {

namespace rtpt {
// Forward Declaration
struct ViewportData;
struct SceneData;
} // namespace rtpt

class RealtimePathTracingPipeline : public RenderPipeline {
  using Renderer = d3d::Renderer;
  using ViewportData = rtpt::ViewportData;
  using SceneData = rtpt::SceneData;

public:
  RealtimePathTracingPipeline(std::weak_ptr<rei::Renderer> renderer);

  ViewportHandle register_viewport(ViewportConfig conf) override;
  void remove_viewport(ViewportHandle viewport) override { /* TODO */ }
  void transform_viewport(ViewportHandle handle, const Camera& camera) override;

  SceneHandle register_scene(SceneConfig conf) override;
  void remove_scene(SceneHandle scene) override { /* TODO */ }
  void add_model(SceneHandle scene, ModelHandle model) override;

  void render(ViewportHandle viewport, SceneHandle scene) override;

protected:
  using ViewportPtr = std::shared_ptr<ViewportData>;
  using ScenePtr = std::shared_ptr<SceneData>;

  std::weak_ptr<Renderer> m_renderer;
  std::unordered_map<ViewportHandle, ViewportPtr> viewports;
  std::unordered_map<SceneHandle, ScenePtr> scenes;

  ShaderHandle pathtracing_shader;
  ShaderArgumentHandle pathtracing_argument;

  // TODO handle expiration
  Renderer* get_renderer() const { return m_renderer.lock().get(); }

  // TODO add validation
  template<typename SharedPtr, typename Handle> 
  typename SharedPtr::element_type* get_ptr(Handle handle, std::unordered_map<Handle, SharedPtr> map) {
    auto found = map.find(handle);
    if (found != map.end()) { return found->second.get(); }
    return nullptr;
  }
  ViewportData* get_viewport(ViewportHandle handle) { return get_ptr(handle, viewports); }
  SceneData* get_scene(SceneHandle handle) { return get_ptr(handle, scenes); }
};

} // namespace rei

#endif
