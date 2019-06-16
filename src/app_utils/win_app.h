#ifndef REI_WIN_APP_H
#define REI_WIN_APP_H

#include <string>

#include <windows.h>

#include "../scene.h"
#include "viewer.h"
#include "win_viewer.h"
#include "../direct3d/d3d_renderer.h"

namespace rei {

class WinApp {

public:
  WinApp(std::wstring title, bool show_fps_in_title, int width, int height);
  ~WinApp() = default;

  void setup(std::shared_ptr<Scene> scene, std::shared_ptr<Camera> camera);

  void run();

protected:
  virtual void on_update();
  virtual void on_render();

private:

  HINSTANCE hinstance;

  Viewer* viewer;
  Renderer* renderer;
  Scene* scene;
  Camera* camera; // default camera
};

}

#endif
