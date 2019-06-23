#if WIN32

#include "win_app.h"

#include <iomanip>
#include <sstream>

using std::make_shared;
using std::make_unique;

namespace rei {

WinApp::WinApp(Config config) : config(config) {
  // NOTE: Handle normally get from WinMain;
  // we dont do that here, because we want to create app with standard main()
  hinstance = GetModuleHandle(nullptr); // handle to the current .exe

  input_bus = make_shared<InputBus>();

  // Default renderer
  auto d3d_renderer = make_shared<d3d::Renderer>(hinstance);
  renderer = d3d_renderer;

  // view the scene
  auto win_viewer = make_shared<WinViewer>(hinstance, config.width, config.height, config.title);
  viewer = win_viewer;
  viewer->init_viewport(*renderer);
  viewer->set_input_bus(input_bus);

  renderer->set_viewport_clear_value(viewer->get_viewport(), config.bg_color);
}

WinApp::~WinApp() {
  log("WinApp terminated.");
}

void WinApp::setup(std::shared_ptr<Scene> scene, std::shared_ptr<Camera> camera) {
  this->scene = scene;
  this->camera = camera;
  renderer->update_viewport_transform(viewer->get_viewport(), *camera);
}

void WinApp::on_update() {
  // Model-centred camera control
  Vec3 acc;
  for (auto& input : input_bus->get<CursorDrag>()) {
    auto mov = *(input.get<CursorDrag>());
    acc += (mov.stop - mov.start);
  }
  const Vec3 focus_center {0, 0, 0};
  const Vec3 up {0, 1, 0};
  camera->rotate_position(focus_center, up, -acc.x / 100);
  camera->rotate_position(focus_center, -camera->right(), -acc.y / 100);
  camera->look_at(focus_center, up);

  // update title
  if (config.show_fps_in_title) {
    using millisecs = std::chrono::duration<float, std::milli>;
    float ms = millisecs(last_frame_time).count();
    float fps = 1000.f / ms;
    std::wstringstream str {};
    str << config.title << " -- frame info: " << std::setw(16) << ms << "ms, " << std::setw(16)
        << fps << "fps";
    viewer->update_title(str.str());
  }

  input_bus->reset();
}

void WinApp::on_render() {
  clock_last_check = clock.now();

  renderer->prepare(*scene);
  ViewportHandle viewport = viewer->get_viewport();
  renderer->update_viewport_transform(viewport, *camera);
  auto culling_result = renderer->cull(viewport, *scene);
  renderer->render(viewport, culling_result);

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
