#ifndef REI_RT_PATH_TRACING_H
#define REI_RT_PATH_TRACING_H

#include <memory>
#include <vector>
#include <unordered_map>

#include "../camera.h"
#include "render_pipeline_base.h"

namespace rei {

namespace rtpt {
// Forward Declaration
struct ViewportData;
struct SceneData;
} // namespace rtpt

// FIXME lazy
namespace d3d {
class Renderer;
}

class RealtimePathTracingPipeline
    : public SimplexPipeline<rtpt::ViewportData, rtpt::SceneData, d3d::Renderer> {
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
  ShaderHandle pathtracing_shader;
  ShaderArgumentHandle pathtracing_argument;
};

} // namespace rei

#endif
