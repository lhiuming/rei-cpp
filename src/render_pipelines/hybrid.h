#ifndef REI_RENDER_PIPELINE_HYBRID_H
#define REI_RENDER_PIPELINE_HYBRID_H

#include <functional>
#include <string>

#include "imgui_pass.h"
#include "render_pipeline_base.h"
#include "stochastic_shadow.h"

namespace rei {

class Renderer;
class ImGUI;
class Scene;
class Camera;
class Geometry;
class Material;

// Forwared decl
namespace hybrid {
struct ViewportData;
struct SceneData;
struct HybridData;
} // namespace hybrid

//
// Hybrid pipeline mixing deferred-rasterizing and raytracing
//
class HybridPipeline : public RenderPipeline {
public:
  struct Context {
    std::weak_ptr<Renderer> renderer;
    SystemWindowID wnd_id;
    std::weak_ptr<Scene> scene;
    std::weak_ptr<Camera> camera;
    std::weak_ptr<ImGUI> imgui;
  };

public:
  HybridPipeline(std::weak_ptr<Renderer> renderer, SystemWindowID wnd_id,
    std::weak_ptr<Scene> scene, std::weak_ptr<Camera> camera);
  HybridPipeline(const Context& context);
  ~HybridPipeline();

  void on_create_geometry(const Geometry& geometry);
  void on_create_material(const Material& material);
  void on_create_model(const Model& model);

  void render(size_t width, size_t height);

private:
  // Configuration
  const int m_frame_buffer_count = 2;
  const bool m_enable_jittering = true;
  const bool m_enable_multibounce = true;
  const bool m_enabled_accumulated_rtrt = true;
  const int m_area_shadow_ssp_per_light = 4;

  // Renderer
  std::weak_ptr<Renderer> m_renderer;

  // Scene & Viewport
  std::weak_ptr<Scene> m_scene;
  std::weak_ptr<Camera> m_camera;
  std::unique_ptr<hybrid::SceneData> m_scene_data;
  std::unique_ptr<hybrid::ViewportData> m_viewport_data;
  std::unique_ptr<hybrid::HybridData> m_hybrid_data;

  ShaderHandle m_gpass_shader;

  ShaderHandle m_base_shading_shader;
  ShaderHandle m_punctual_lighting_shader;
  ShaderHandle m_area_lighting_shader;

  ShaderHandle m_multibounce_shader;

  ShaderHandle m_taa_shader;
  ShaderHandle m_blit_shader;

  BufferHandle m_per_render_buffer;

  StochasticShadowPass m_sto_shadow_pass;
  ImGuiPass m_imgui_pass;

  void update_scene();
  void update_viewport(size_t width, size_t height);
  void update_hybrid();

  ShaderArgumentHandle fetch_direct_punctual_lighting_arg(
    Renderer& renderer, hybrid::SceneData& scene, int cb_index);
  ShaderArgumentHandle fetch_direct_area_lighting_arg(
    Renderer& renderer, hybrid::SceneData& scene, int cb_index);
};

} // namespace rei

#endif