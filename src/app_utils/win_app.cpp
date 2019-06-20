#if WIN32

#include "win_app.h"

#include <sstream>
#include <iomanip>

using std::make_shared;

namespace rei {

WinApp::WinApp(Config config) : config(config) {
  // NOTE: Handle normally get from WinMain;
  // we dont do that here, because we want to create app with standard main()
  hinstance = GetModuleHandle(nullptr); // handle to the current .exe

  // Default renderer
  auto d3d_renderer = make_shared<d3d::Renderer>(hinstance);
  renderer = d3d_renderer;

  // Default scene
  scene = make_shared<StaticScene>();
  camera = make_shared<Camera>();

  // view the scene
  auto win_viewer = make_shared<WinViewer>(hinstance, config.width, config.height, config.title);
  viewer = win_viewer;
  viewer->init_viewport(*renderer);
}

WinApp::~WinApp() {
  log("WinApp terminated.");
}

void WinApp::setup(std::shared_ptr<Scene> scene, std::shared_ptr<Camera> camera) {
  this->scene = scene;
  this->camera = camera;
  renderer->set_scene(scene);
  renderer->update_viewport_transform(viewer->get_viewport(), *camera);
}

void WinApp::on_update() {
  // scene->update();
  camera->move(0.01, 0.0, 0.0);
  camera->set_target(Vec3(0.0, 0.0, 0.0));

  // update title
  if (config.show_fps_in_title) {
    using millisecs = std::chrono::duration<float, std::milli>;
    float ms = millisecs(last_frame_time).count();
    float fps = 1000.f / ms;
    std::wstringstream str {};
    str << config.title << " -- frame info: " 
      << std::setw(16) << ms << "ms, " 
      << std::setw(16) << fps << "fps";
    viewer->update_title(str.str());
  }
}

void WinApp::on_render() {
  clock_last_check = clock.now();
  renderer->render(viewer->get_viewport());
  last_frame_time = clock.now() - clock_last_check;
}

void WinApp::run() {
  // Check event and do update&render
  MSG msg;
  ZeroMemory(&msg, sizeof(MSG));
  while (true) {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) // passge message to WndProc
    {
      if (msg.message == WM_QUIT) break;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else {
      on_update();
      on_render();
      // Flip the buffer
    }
  } // end while
}

} // namespace rei

#endif
