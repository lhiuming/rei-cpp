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

class Renderer;

struct GraphicData {
  GraphicData(Renderer* owner) : owner(owner) {}
  const Renderer const* owner; // as a marker
};

struct ViewportData : GraphicData {
  using GraphicData::GraphicData;
  bool enable_vsync = false;
};

struct ShaderData : GraphicData {
  using GraphicData::GraphicData;
};

struct MaterialData : GraphicData {
  using GraphicData::GraphicData;
};

struct ModelData : GraphicData {
  using GraphicData::GraphicData;
  Mat4 transform;
};

struct GeometryData : GraphicData {
  using GraphicData::GraphicData;
};

// Renderer ///////////////////////////////////////////////////////////////////
// The base class
////

class Renderer : private NoCopy {
protected:
public:
  using ViewportHandle = std::shared_ptr<ViewportData>;
  using ShaderHandle = std::shared_ptr<ShaderData>;
  using GeometryHandle = std::shared_ptr<GeometryData>;
  using ModelHandle = std::shared_ptr<ModelData>;

public:
  Renderer();
  virtual ~Renderer() {};

  virtual ViewportHandle create_viewport(SystemWindowID window_id, int width, int height) = 0;
  virtual void set_viewport_clear_value(ViewportHandle viewport, Color color) = 0;
  virtual void update_viewport_vsync(ViewportHandle viewport, bool enabled_vsync) = 0;
  virtual void update_viewport_size(ViewportHandle viewport, int width, int height) = 0;
  virtual void update_viewport_transform(ViewportHandle viewport, const Camera& camera) = 0;

  virtual GeometryHandle create_geometry(const Geometry& geo) = 0;
  virtual void set_scene(std::shared_ptr<const Scene> scene) { this->scene = scene; }
  virtual ShaderHandle create_shader(std::wstring shader_path) = 0;

  [[deprecated]] virtual void set_camera(std::shared_ptr<const Camera> camera) { DEPRECATED }

  [[deprecated]] virtual void render() {}

  virtual void render(ViewportHandle viewport) = 0;

protected:
  std::shared_ptr<const Scene> scene;
};

// A cross-platform renderer factory
[[deprecated]] std::shared_ptr<Renderer> makeRenderer();

} // namespace rei

#endif
