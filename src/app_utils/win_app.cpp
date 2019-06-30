#if WIN32

#include "win_app.h"

#include <iomanip>
#include <sstream>

#include "../debug.h"
#include "../rmath.h"

using std::make_shared;
using std::make_unique;
using std::shared_ptr;
using std::unique_ptr;

namespace rei {

WinApp::WinApp(Config config, unique_ptr<Renderer>&& renderer) : config(config), m_renderer(std::move(renderer)) {
  // NOTE: Handle normally get from WinMain;
  // we dont do that here, because we want to create app with standard main()
  hinstance = get_hinstance(); // handle to the current .exe

  m_input_bus = make_unique<InputBus>();

  // Default renderer
  if (m_renderer == nullptr) { m_renderer = make_unique<d3d::Renderer>(hinstance); }

  // Default viewer
  m_viewer = make_unique<WinViewer>(hinstance, config.width, config.height, config.title);
  m_viewer->init_viewport(*m_renderer);
  m_viewer->set_input_bus(m_input_bus);
  m_renderer->set_viewport_clear_value(m_viewer->get_viewport(), config.bg_color);

  // Create default scene and camera
  m_scene = make_unique<Scene>();
  m_camera = make_unique<Camera>(Vec3 {0, 0, 10}, Vec3 {0, 0, -1});
  m_camera->set_aspect(config.width, config.height);
}

WinApp::~WinApp() {
  log("WinApp terminated.");
}

void WinApp::setup(Scene&& scene, Camera&& camera) {
  if (is_started) {
    REI_WARNING("App setup is called after started");
    return;
  }
  initialize_scene();
}

void WinApp::initialize_scene() {
  for (ModelPtr& m : this->m_scene->get_models()) {
    REI_ASSERT(m);

    // register geometry
    GeometryPtr geo = m->get_geometry();
    if (geo && geo->get_graphic_handle() == nullptr) {
      GeometryHandle g_handle = m_renderer->create_geometry(*geo);
      geo->set_graphic_handle(g_handle);
    }

    // register material&shader
    MaterialPtr mat = m->get_material();
    if (mat && mat->get_graphic_handle() == nullptr) { REI_NOT_IMPLEMENTED }

    ModelHandle h_model = m_renderer->create_model(*m);
    m->set_rendering_handle(h_model);
  }

  m_scene->set_graphic_handle(m_renderer->build_enviroment(*m_scene));
}

void WinApp::start() {
  on_start();

  initialize_scene();

  is_started = true;
}

void WinApp::on_start() {
  if (config.enable_grid_line) {
    // todo create and draw grid line
  }
}

void WinApp::update() {
  // callback
  on_update();

  // Finalize
  m_input_bus->reset();
}

void WinApp::on_update() {
  update_camera_control();
  update_title();
}

void WinApp::update_camera_control() {
  if (config.default_camera_control_enabled) {
    const Vec3 focus_center {0, 0, 0};
    const Vec3 up {0, 1, 0};

    Vec3 acc;
    for (auto& input : m_input_bus->get<CursorDrag>()) {
      auto mov = *(input.get<CursorDrag>());
      acc += (mov.stop - mov.start);
    }
    if (acc.norm2() > 0.0001) {
      const double rot_range = pi;
      m_camera->rotate_position(focus_center, up, -acc.x / m_viewer->width() * rot_range);
      m_camera->rotate_position(
        focus_center, -m_camera->right(), -acc.y / m_viewer->height() * rot_range);
      m_camera->look_at(focus_center, up);
    }

    double zoom_in = 0;
    for (auto& input : m_input_bus->get<Zoom>()) {
      auto zoom = *(input.get<Zoom>());
      zoom_in += zoom.delta;
    }
    if (std::abs(zoom_in) != 0) {
      const double ideal_dist = 8;
      const double block_dist = 4;
      double curr_dist = (m_camera->position() - focus_center).norm();
      double scale = std::abs((curr_dist - block_dist) / (ideal_dist - block_dist));
      if (zoom_in < 0) scale = (std::max)(scale, 0.1);
      m_camera->move(0, 0, zoom_in * scale);
    }
  }
}

void WinApp::update_title() {
  if (config.show_fps_in_title && !m_viewer->is_destroyed()) {
    using millisecs = std::chrono::duration<float, std::milli>;
    float ms = millisecs(last_frame_time).count();
    float fps = 1000.f / ms;
    std::wstringstream str {};
    str << config.title << " -- frame info: " << std::setw(16) << ms << "ms, " << std::setw(16)
        << fps << "fps";
    m_viewer->update_title(str.str());
  }
}

void WinApp::render() {
  clock_last_check = clock.now();
  on_render();
  last_frame_time = clock.now() - clock_last_check;
}

void WinApp::on_render() {
  m_renderer->prepare(*m_scene);
  ViewportHandle viewport = m_viewer->get_viewport();
  m_renderer->update_viewport_transform(viewport, *m_camera);
  auto culling_result = m_renderer->cull(viewport, *m_scene);
  m_renderer->render(viewport, culling_result);
}

void WinApp::run() {
  start();

  // Check event and do update&render
  MSG msg;
  ZeroMemory(&msg, sizeof(MSG));
  while (true) {
    // pass message to WndProc
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) break;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    // simple one tick
    update();
    render();
  } // end while
}

} // namespace rei

#endif
