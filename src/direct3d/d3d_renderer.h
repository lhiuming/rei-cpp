#ifndef REI_DIRECT3D_D3D_RENDERER_H
#define REI_DIRECT3D_D3D_RENDERER_H

#if DIRECT3D_ENABLED

#include <memory>

#include "../algebra.h"
#include "../camera.h"
#include "../model.h"
#include "../renderer.h"
#include "../scene.h"

/*
 * direct3d/d3d_renderer.h
 * Implement a Direct3D11-based renderer.
 */

class D3D11_VIEWPORT;

namespace rei {

class D3DDeviceResources;
class D3DViewportResources;

class D3DRenderer : public Renderer {
protected:
  struct D3DViewportData : public ViewportData {
    using ViewportData::ViewportData;
    std::weak_ptr<D3D11_VIEWPORT> d3d_viewport;
    std::weak_ptr<D3DViewportResources> viewport_resources;
  };

public:
  D3DRenderer(HINSTANCE hinstance);
  ~D3DRenderer() override;

  ViewportHandle create_viewport(SystemWindowID window_id, int width, int height) override;
  void update_viewport_vsync(ViewportHandle viewport, bool enabled_vsync) override;
  void update_viewport_size(ViewportHandle viewport, int width, int height) override;
  void update_viewport_transform(ViewportHandle viewport, const Camera& camera) override;

  void set_scene(std::shared_ptr<const Scene> scene) override;

  void render(ViewportHandle viewport) override;

protected:
  HINSTANCE hinstance;
  std::unique_ptr<D3DDeviceResources> device_resources;
  std::vector<std::shared_ptr<D3DViewportResources>> viewport_resources;
  std::vector<std::shared_ptr<D3D11_VIEWPORT>> viewports;

  void render_default_scene(D3DViewportData& viewport_resources);
  void render_meshes(D3DViewportData& viewport_resources);
  void rasterize_triangle(const Mesh::Triangle& tri, const Mat4& trans);

  inline std::shared_ptr<D3DViewportData> get_viewport(ViewportHandle h_viewport) {
    if (h_viewport && (h_viewport->owner == this))
      return std::static_pointer_cast<D3DViewportData>(h_viewport);
    return nullptr;
  }

};

}

#endif

#endif
