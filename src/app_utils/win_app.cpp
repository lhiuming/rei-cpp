#if WIN32

#include "win_app.h"

namespace rei {

WinApp::WinApp(std::wstring title, bool show_fps_in_title, int width, int height) {
  // NOTE: Handle normally get from WinMain;
  // we dont do that here, because we want to create app with standard main()
  hinstance = GetModuleHandle(nullptr); // handle to the current .exe

  // Default renderer
  auto d3d_renderer = new D3DRenderer(hinstance);
  renderer = d3d_renderer;

  // Default scene
  scene = new StaticScene();
  camera = new Camera();

  // view the scene
  auto win_viewer = new WinViewer(hinstance, width, height, title);
  viewer = win_viewer;
  viewer->init_viewport(*renderer);
}

void WinApp::setup(std::shared_ptr<Scene> scene, std::shared_ptr<Camera> camera) {
  scene = scene;
  camera = camera;

  renderer->set_scene(scene);
  renderer->update_viewport_transform(viewer->get_viewport(), *camera);
}

void WinApp::on_update() {
  // scene->update();
  camera->move(0.01, 0.0, 0.0);
  camera->set_target(Vec3(0.0, 0.0, 0.0));
}

void WinApp::on_render() {
  renderer->render(viewer->get_viewport());
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

}

#endif
