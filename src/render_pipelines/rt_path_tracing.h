#ifndef REI_RT_PATH_TRACING_H
#define REI_RT_PATH_TRACING_H

#include <memory>

#include "../camera.h"
#include "../render_pipeline.h";
// TODO for laziness
#include "../direct3d/d3d_renderer.h";

namespace rei {

namespace rtpt {
struct ViewportData {
  // Window viewport
  SwapchainHandle swapchain;
  ScreenTransformHandle view_transform;
  size_t width, height;

  // Camera info
  bool camera_matrix_dirty = true;
  Mat4 view_matrix;
  Mat4 view_proj_matrix;

  // Raytracing resources
  BufferHandle raytracing_output_buffer;
  BufferHandle per_render_buffer;
};

struct SceneData {
  // Raytracing resources
  BufferHandle tlas_buffer;
  BufferHandle shader_table;
  // Model proxies
  //std::vector<MaterialHandle> dirty_materials;
  std::vector<ModelHandle> dirty_models;
};

} // namespace rtpt

class RealtimePathTracingPipeline : RenderPipeline<rtpt::ViewportData, rtpt::SceneData> {
  using Renderer = d3d::Renderer;
  using ViewportData = rtpt::ViewportData;
  using SceneData = rtpt::SceneData;

public:
  RealtimePathTracingPipeline(std::weak_ptr<Renderer> renderer);

  ViewportHandle register_viewport(ViewportConfig conf) override;
  void transform_viewport(ViewportHandle handle, const Camera& camera) override;

  SceneHandle register_scene(SceneConfig conf) override;
  void add_model(SceneHandle scene, ModelHandle model) override;

  void render(ViewportHandle viewport, SceneHandle scene) override;

protected:
  std::weak_ptr<Renderer> m_renderer;
  std::vector<std::shared_ptr<ViewportData>> viewports;
  std::vector<std::shared_ptr<SceneData>> scenes;

  ShaderHandle pathtracing_shader;
  ShaderArgumentHandle pathtracing_argument;

  // TODO handle expiration
  Renderer* get_renderer() const { return m_renderer.lock().get(); }

  // TODO add validation
  ViewportData& get_viewport(ViewportHandle handle) const { return *(handle.lock()); }
  SceneData& get_scene(SceneHandle handle) const { return *(handle.lock()); }
};

} // namespace rei

#endif
