#ifndef REI_RENDER_PIPELINE_BASE_H
#define REI_RENDER_PIPELINE_BASE_H

#include <memory>
#include <unordered_map>

#include "../graphic_handle.h"
#include "../renderer.h"

namespace rei {

struct ViewportConfig {
  size_t width, height;
  SystemWindowID window_id;
};

struct SceneConfig {
  const Scene* scene;
};

class RenderPipeline {
  using PtrType = std::uintptr_t;

public:
  using ViewportHandle = PtrType;
  using SceneHandle = PtrType;

  virtual ViewportHandle register_viewport(ViewportConfig conf) = 0;
  virtual void remove_viewport(ViewportHandle viewport) = 0;

  virtual void transform_viewport(ViewportHandle handle, const Camera& camera) = 0;

  virtual SceneHandle register_scene(SceneConfig conf) = 0;
  virtual void remove_scene(SceneHandle scene) = 0;

  virtual void add_model(SceneHandle scene, const Model& model, Scene::ModelUID model_id) = 0;
  virtual void update_model(SceneHandle scene, const Model& model, Scene::ModelUID model_id) = 0;

  virtual void render(ViewportHandle viewport, SceneHandle scene) = 0;
};

template <typename TViewport, typename TScene, typename TRenderer = Renderer>
class SimplexPipeline : public RenderPipeline {
protected:
  using RendererPtr = std::weak_ptr<TRenderer>;

public:
  SimplexPipeline(std::weak_ptr<TRenderer> renderer) : m_renderer(renderer) {}

protected:
  using ViewportPtr = std::shared_ptr<TViewport>;
  using ScenePtr = std::shared_ptr<TScene>;

  std::weak_ptr<TRenderer> m_renderer;
  std::unordered_map<ViewportHandle, ViewportPtr> viewports;
  std::unordered_map<SceneHandle, ScenePtr> scenes;

  // TODO handle expiration
  TRenderer* get_renderer() const { return m_renderer.lock().get(); }

  ViewportHandle add_viewport(TViewport&& viewport) {
    ViewportPtr ptr = std::make_shared<TViewport>(viewport);
    ViewportHandle handle = ViewportHandle(ptr.get());
    viewports.insert({handle, ptr});
    return handle;
  }

  SceneHandle add_scene(TScene&& scene) {
    ScenePtr ptr = std::make_shared<TScene>(scene);
    SceneHandle handle = SceneHandle(ptr.get());
    scenes.insert({handle, ptr});
    return handle;
  }

  // TODO add validation
  template <typename SharedPtr, typename Handle>
  typename SharedPtr::element_type* get_ptr(
    Handle handle, std::unordered_map<Handle, SharedPtr> map) {
    auto found = map.find(handle);
    if (found != map.end()) { return found->second.get(); }
    return nullptr;
  }
  TViewport* get_viewport(ViewportHandle handle) { return get_ptr(handle, viewports); }
  TScene* get_scene(SceneHandle handle) { return get_ptr(handle, scenes); }
};

} // namespace rei

#endif
