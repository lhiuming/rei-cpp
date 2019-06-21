#ifndef REI_DIRECT3D_D3D_RENDERER_H
#define REI_DIRECT3D_D3D_RENDERER_H

#if DIRECT3D_ENABLED

#include <memory>
#include <array>

#include "../common.h"
#include "../algebra.h"
#include "../camera.h"
#include "../model.h"
#include "../scene.h"
#include "../renderer.h"

/*
 * direct3d/d3d_renderer.h
 * Implement a Direct3D11-based renderer.
 */

namespace rei {

namespace d3d {

class DeviceResources;
class ViewportResources;
struct ShaderData;
struct ViewportData;

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

  void set_scene(std::shared_ptr<const Scene> scene) override;

  ShaderHandle create_shader(std::wstring shader_path) override;
  GeometryHandle create_geometry(const Geometry& geometry) override;

  void render(ViewportHandle viewport) override;

protected:
  HINSTANCE hinstance;

  std::unique_ptr<DeviceResources> device_resources;
  std::vector<std::shared_ptr<ViewportResources>> viewport_resources_lib;

  void render(ViewportData& viewport);

  void render_default_scene(ViewportData& viewport_resources);
  void render_meshes(ViewportData& viewport_resources);
  void rasterize_triangle(const Mesh::Triangle& tri, const Mat4& trans);

  std::shared_ptr<ViewportData> get_viewport(ViewportHandle h_viewport);
};

} // namespace d3d

} // namespace rei

#endif

#endif
