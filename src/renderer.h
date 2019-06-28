#ifndef REI_RENDERER_H
#define REI_RENDERER_H

#include <cstddef>
#include <memory>

#if WIN32
#include <windows.h>
#endif

#include "camera.h"
#include "common.h"
#include "model.h"
#include "scene.h"

#include "graphic_handle.h"

/*
 * renderer.h
 * Define the Renderer interface. Implementation is platform-specific.
 * (see opengl/renderer.h and d3d/renderer.h)
 */

namespace rei {

struct SystemWindowID {
  enum Platform {
    Win,
    Offscreen,
  } platform;
  union {
    HWND hwnd;
  } value;
};

// Renderer ///////////////////////////////////////////////////////////////////
// The base class
////

class Renderer : private NoCopy {
public:
  Renderer();
  virtual ~Renderer() {};

  virtual ViewportHandle create_viewport(SystemWindowID window_id, int width, int height) = 0;
  virtual void set_viewport_clear_value(ViewportHandle viewport, Color color) = 0;
  virtual void update_viewport_vsync(ViewportHandle viewport, bool enabled_vsync) = 0;
  virtual void update_viewport_size(ViewportHandle viewport, int width, int height) = 0;
  virtual void update_viewport_transform(ViewportHandle viewport, const Camera& camera) = 0;

  virtual ShaderHandle create_shader(std::wstring shader_path) = 0;
  virtual GeometryHandle create_geometry(const Geometry& geo) = 0;
  virtual ModelHandle create_model(const Model& model) = 0;
  virtual SceneHandle build_enviroment(const Scene& scene) = 0;

  virtual void prepare(Scene& scene) = 0;
  virtual CullingResult cull(ViewportHandle viewport, const Scene& scene) = 0;
  virtual void render(ViewportHandle viewport, CullingResult culling_result) = 0;

protected:
  // Convert from handle to data
  template <typename Handle, typename Data>
  std::shared_ptr<Data> get_data(Handle handle) {
    if (handle && (handle->owner == this)) {
      // Pointer to forward-delcared class cannot be static up-cast, so we have to use reinterpret cast
      //return std::static_pointer_cast<Data>(handle);
      return std::reinterpret_pointer_cast<Data>(handle);
    }
    return nullptr;
  }
};

// A cross-platform renderer factory
[[deprecated]] std::shared_ptr<Renderer> makeRenderer();

} // namespace rei

#endif
