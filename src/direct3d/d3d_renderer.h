#ifndef REI_DIRECT3D_D3D_RENDERER_H
#define REI_DIRECT3D_D3D_RENDERER_H

#if DIRECT3D_ENABLED

#include <memory>

#include "../algebra.h"
#include "../camera.h"
#include "../model.h"
#include "../scene.h"
#include "../renderer.h"

//#include "d3d_device_resources.h"
//#include "d3d_viewport_resources.h"

/*
 * direct3d/d3d_renderer.h
 * Implement a Direct3D11-based renderer.
 */

class D3D11_VIEWPORT;

namespace rei {

namespace d3d {

class DeviceResources;
class ViewportResources;
struct ShaderResources;
struct MeshResource;

class Renderer : public rei::Renderer {
  using Self = typename Renderer;
  using Base = typename rei::Renderer;

protected:
  struct D3DViewportData : Base::ViewportData {
    using ViewportData::ViewportData;
    std::weak_ptr<D3D11_VIEWPORT> d3d_viewport;
    std::weak_ptr<ViewportResources> viewport_resources;
  };

  struct D3DShaderData : Base::ShaderData {
    using ShaderData::ShaderData;
    std::weak_ptr<ShaderResources> resources;
  };

  struct GeometryData : Base::GeometryData {
    using Base::GeometryData::GeometryData;
    // Bound bound;
  };

  struct MeshData : GeometryData {
    using GeometryData::GeometryData;
    std::weak_ptr<MeshResource> mesh_res;
  };

  struct D3DModelData : Base::ModelData {
    // Bound aabb;
  };

  // Data proxy for all obejct in a scene
  struct SceneData {};

public:
  Renderer(HINSTANCE hinstance);
  ~Renderer() override;

  ViewportHandle create_viewport(SystemWindowID window_id, int width, int height) override;
  void update_viewport_vsync(ViewportHandle viewport, bool enabled_vsync) override;
  void update_viewport_size(ViewportHandle viewport, int width, int height) override;
  void update_viewport_transform(ViewportHandle viewport, const Camera& camera) override;

  void set_scene(std::shared_ptr<const Scene> scene) override;

  ShaderID create_shader(std::wstring shader_path) override;
  GeometryID create_geometry(const Geometry& geometry) override;

  void render(ViewportHandle viewport) override;

protected:
  HINSTANCE hinstance;

  std::unique_ptr<DeviceResources> device_resources;
  std::vector<std::shared_ptr<ViewportResources>> viewport_resources_lib;
  std::vector<std::shared_ptr<D3D11_VIEWPORT>> viewport_lib;

  std::vector<std::shared_ptr<ShaderResources>> shader_resource_lib;

  void render(D3DViewportData& viewport);

  void render_default_scene(D3DViewportData& viewport_resources);
  void render_meshes(D3DViewportData& viewport_resources);
  void rasterize_triangle(const Mesh::Triangle& tri, const Mat4& trans);

  inline std::shared_ptr<D3DViewportData> get_viewport(ViewportHandle h_viewport) {
    if (h_viewport && (h_viewport->owner == this))
      return std::static_pointer_cast<D3DViewportData>(h_viewport);
    return nullptr;
  }
};

} // namespace d3d

} // namespace rei

#endif

#endif
