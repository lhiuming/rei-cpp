#ifndef REI_DIRECT3D_D3D_RENDERER_H
#define REI_DIRECT3D_D3D_RENDERER_H

#if DIRECT3D_ENABLED

#include <array>
#include <memory>

#include "../algebra.h"
#include "../camera.h"
#include "../common.h"
#include "../model.h"
#include "../renderer.h"
#include "../scene.h"

/*
 * direct3d/d3d_renderer.h
 * Implement a Direct3D11-based renderer.
 */
#include "d3d_common_resources.h"

namespace rei {

namespace d3d {

class DeviceResources;
class ViewportResources;
struct ShaderData;
struct ViewportData;
struct CullingData;

class Renderer : public rei::Renderer {
  using Self = typename Renderer;
  using Base = typename rei::Renderer;

public:
  Renderer(HINSTANCE hinstance);
  ~Renderer() override;

  ViewportHandle create_viewport(SystemWindowID window_id, int width, int height) override;
  void set_viewport_clear_value(ViewportHandle viewport, Color color) override;
  void update_viewport_vsync(ViewportHandle viewport, bool enabled_vsync) override;
  void update_viewport_size(ViewportHandle viewport, int width, int height) override;
  void update_viewport_transform(ViewportHandle viewport, const Camera& camera) override;

  ShaderHandle create_shader(std::wstring shader_path) override;
  GeometryHandle create_geometry(const Geometry& geometry) override;
  ModelHandle create_model(const Model& model) override;

  void prepare(Scene& scene) override;
  CullingResult cull(ViewportHandle viewport, const Scene& scene) override;
  void render(ViewportHandle viewport, CullingResult culling_result) override;

protected:
  HINSTANCE hinstance;

  std::shared_ptr<DeviceResources> device_resources; // shared with viewport resources
  std::vector<std::shared_ptr<ViewportResources>> viewport_resources_lib;

  std::shared_ptr<GeometryData> default_geometry;
  std::shared_ptr<ShaderData> default_shader;
  std::shared_ptr<MaterialData> default_material;
  std::shared_ptr<ModelData> debug_model;

  bool is_uploading_resources = false;

  const bool draw_debug_model = false;

  void upload_resources();
  void render(ViewportData& viewport, CullingData& culling);

  struct ModelDrawTask {
    std::size_t model_num;
    const ModelData* models;
    const Mat4* view_proj;
    const RenderTargetSpec* target_spec;
  };

  void draw_meshes(ID3D12GraphicsCommandList& cmd_list, ModelDrawTask& task);

  void create_default_assets();

  std::shared_ptr<ViewportData> to_viewport(ViewportHandle h) { return get_data<ViewportHandle, ViewportData>(h); }
  std::shared_ptr<CullingData> to_culling(CullingResult h) { return get_data<CullingResult, CullingData>(h); }
  std::shared_ptr<ModelData> to_model(ModelHandle h) { return get_data<ModelHandle, ModelData>(h); }
  std::shared_ptr<ShaderData> to_shader(ShaderHandle h) { return get_data<ShaderHandle, ShaderData>(h); }
  std::shared_ptr<GeometryData> to_geometry(GeometryHandle h) { return get_data<GeometryHandle, GeometryData>(h); }
  std::shared_ptr<MaterialData> to_material(MaterialHandle h) { return get_data<MaterialHandle, MaterialData>(h); }

};

} // namespace d3d

} // namespace rei

#endif

#endif
