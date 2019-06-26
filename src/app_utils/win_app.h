#ifndef REI_WIN_APP_H
#define REI_WIN_APP_H

#include <memory>
#include <string>
#include <chrono>

#include <windows.h>

#include "../color.h"
#include "../scene.h"
#include "../input.h"
#include "viewer.h"
#include "win_viewer.h"
#include "../direct3d/d3d_renderer.h"

namespace rei {

class WinApp {
public:
  struct Config {
    std::wstring title = L"Unmaed App";
    int height = 720;
    int width = 480;
    bool show_fps_in_title = true;
    bool enable_vsync = false;
    Color bg_color = Colors::ayanami_blue;
  };

public:
  WinApp(Config config);
  virtual ~WinApp();

  void setup(Scene&& scene, Camera&& camera);

  void run();

protected:
  virtual void on_update();
  virtual void on_render();

private:
  Config config;

  HINSTANCE hinstance = NULL;

  std::shared_ptr<InputBus> input_bus;

  std::unique_ptr<Viewer> viewer;
  std::unique_ptr<Renderer> renderer;
  std::unique_ptr<Scene> scene;
  std::unique_ptr<Camera> camera;

  using clock_t = std::chrono::high_resolution_clock;
  clock_t clock;
  clock_t::time_point clock_last_check;
  clock_t::duration last_frame_time;
};

}

#endif
