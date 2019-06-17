#ifndef REI_WIN_APP_H
#define REI_WIN_APP_H

#include <string>
#include <chrono>

#include <windows.h>

#include "../scene.h"
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
  };

public:
  WinApp(Config config);
  ~WinApp() = default;

  void setup(std::shared_ptr<Scene> scene, std::shared_ptr<Camera> camera);

  void run();

protected:
  virtual void on_update();
  virtual void on_render();

private:
  Config config;

  HINSTANCE hinstance = NULL;

  Viewer* viewer = nullptr;
  Renderer* renderer = nullptr;
  Scene* scene = nullptr;
  Camera* camera = nullptr;

  using clock_t = std::chrono::high_resolution_clock;
  clock_t clock;
  clock_t::time_point clock_last_check;
  clock_t::duration last_frame_time;
};

}

#endif
