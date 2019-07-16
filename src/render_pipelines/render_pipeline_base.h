#ifndef REI_RENDER_PIPELINE_BASE_H
#define REI_RENDER_PIPELINE_BASE_H

#include <memory>

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

  virtual SceneHandle register_scene(SceneConfig conf) = 0;
  virtual void remove_scene(SceneHandle scene) = 0;
  virtual void add_model(SceneHandle scene, ModelHandle model) = 0;

  virtual void transform_viewport(ViewportHandle handle, const Camera& camera) = 0;

  virtual void render(ViewportHandle viewport, SceneHandle scene) = 0;
};

} // namespace rei

#endif
