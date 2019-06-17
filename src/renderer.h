#ifndef REI_RENDERER_H
#define REI_RENDERER_H

#include <cstddef>
#include <memory>

#if WIN32
#include <windows.h>
#endif

#include "common.h"
#include "camera.h"
#include "model.h"
#include "scene.h"

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
protected:
  struct ViewportData {
    ViewportData(Renderer* owner) : owner(owner) {}
    bool enable_vsync = false;
    Mat4 view_proj = Mat4::I();
    const Renderer const* owner; // as a marker
  };

public:
  using ViewportHandle = std::shared_ptr<ViewportData>;

public:
  Renderer();
  virtual ~Renderer() {};

  virtual ViewportHandle create_viewport(SystemWindowID window_id, int width, int height) = 0;
  virtual void update_viewport_vsync(ViewportHandle viewport, bool enabled_vsync) = 0;
  virtual void update_viewport_size(ViewportHandle viewport, int width, int height) = 0;
  virtual void update_viewport_transform(ViewportHandle viewport, const Camera& camera) = 0;

  virtual void set_scene(std::shared_ptr<const Scene> scene) { this->scene = scene; }
  
  [[deprecated]]
  virtual void set_camera(std::shared_ptr<const Camera> camera) { DEPRECATE }

  [[deprecated]] virtual void render() {}

  virtual void render(ViewportHandle viewport) = 0;

protected:
  std::shared_ptr<const Scene> scene;
};

// A cross-platform renderer factory
[[deprecated]]
std::shared_ptr<Renderer> makeRenderer();

}

#endif
