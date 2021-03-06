#ifndef REI_WIN_APP_H
#define REI_WIN_APP_H

#include <chrono>
#include <memory>
#include <string>

#include <windows.h>

#include "../color.h"
#include "../input.h"
#include "../render_pipeline.h"
#include "../renderer.h"
#include "../scene.h"
//#include "app.h"
#include "win_viewer.h"

namespace rei {

class WinApp {
public:
  enum class RenderMode {
    Rasterization,
    RealtimeRaytracing,
    Hybrid,
  };

  struct Config {
    std::wstring title = L"Unmaed App";
    int height = 720;
    int width = 480;
    bool show_fps_in_title = true;
    bool enable_vsync = false;
    Color bg_color = Colors::ayanami_blue;
    RenderMode render_mode = RenderMode::RealtimeRaytracing;
    bool default_camera_control_enabled = true;
    bool enable_grid_line = true;
  };

  WinApp(Config config);
  virtual ~WinApp();

  void run();

  // init helper
  static inline HINSTANCE get_hinstance() { return GetModuleHandle(nullptr); }

protected:
  Config config;

  Viewer& viewer() const { return *m_viewer; }
  Renderer& renderer() const { return *m_renderer; }
  Scene& scene() const { return *m_scene; }
  Camera& camera() const { return *m_camera; }
  InputBus& input_bus() const { return *m_input_bus; }

  virtual void on_start();
  virtual void on_update();
  virtual void on_render();

  // some useful methods
  void update_camera_control();
  void update_title();

private:
  HINSTANCE hinstance = NULL;

  std::shared_ptr<Renderer> m_renderer;
  std::unique_ptr<Viewer> m_viewer;
  std::unique_ptr<Scene> m_scene;
  std::unique_ptr<Camera> m_camera;
  std::shared_ptr<InputBus> m_input_bus;

  std::shared_ptr<RenderPipeline> m_pipeline;
  RenderPipeline::ViewportHandle m_viewport_h = 0;
  RenderPipeline::SceneHandle m_scene_h = 0;

  bool is_started = false;

  using clock_t = std::chrono::high_resolution_clock;
  clock_t clock;
  clock_t::time_point clock_last_check;
  clock_t::duration last_frame_time;

  // camera control
  Vec3 camera_target;

  void start();
  void initialize_scene(); // TODO should be more elegant
  void update();
  void render();
};

} // namespace rei

#endif
