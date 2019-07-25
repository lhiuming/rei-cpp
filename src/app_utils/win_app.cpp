#if WIN32

#include "win_app.h"

#include <iomanip>
#include <sstream>

#include "../debug.h"
#include "../rmath.h"

#include "../direct3d/d3d_renderer.h"

using std::make_shared;
using std::make_unique;
using std::shared_ptr;
using std::unique_ptr;

namespace rei {

WinApp::WinApp(Config config) : config(config) {
  // NOTE: Handle normally get from WinMain;
  // we dont do that here, because we want to create app with standard main()
  hinstance = get_hinstance(); // handle to the current .exe

  m_input_bus = make_unique<InputBus>();

  // Default renderer & pipeline
  {
    shared_ptr<d3d::Renderer> d3d_renderer;
    d3d::Renderer::Options r_opts = {};
    d3d_renderer = make_shared<d3d::Renderer>(hinstance, r_opts);
    m_renderer = d3d_renderer;
    switch (config.render_mode) {
      case RenderMode::Rasterization:
        m_pipeline = make_shared<DeferredPipeline>(d3d_renderer);
        break;
      case RenderMode::RealtimeRaytracing:
        m_pipeline = make_shared<RealtimePathTracingPipeline>(d3d_renderer);
        break;
      case RenderMode::Hybrid:
      default:
        m_pipeline = make_shared<HybridPipeline>(d3d_renderer);
        break;
    }
  }

  // Default viewer
  m_viewer = make_unique<WinViewer>(hinstance, config.width, config.height, config.title);
  m_viewer->set_input_bus(m_input_bus);
  {
    ViewportConfig conf = {};
    conf.window_id = m_viewer->get_window_id();
    conf.width = config.width;
    conf.height = config.height;
    m_viewport_h = m_pipeline->register_viewport(conf);
  }
  // m_renderer->set_viewport_clear_value(m_viewer->get_viewport(), config.bg_color);

  // Create default scene and camera
  m_scene = make_unique<Scene>();
  m_camera = make_unique<Camera>(Vec3 {0, 0, 10}, Vec3 {0, 0, -1});
  m_camera->set_aspect(config.width, config.height);
}

WinApp::~WinApp() {
  log("WinApp terminated.");
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
    // MaterialPtr mat = m->get_material();
    // if (mat && mat->get_graphic_handle() == nullptr) { REI_NOT_IMPLEMENTED }

    ModelHandle h_model = m_renderer->create_model(*m);
    m->set_rendering_handle(h_model);
  }

  {
    SceneConfig conf {};
    conf.scene = m_scene.get();
    m_scene_h = m_pipeline->register_scene(conf);
  }
}

void WinApp::start() {
  on_start();

  initialize_scene();

  is_started = true;
}

void WinApp::on_start() {
  // init camera control
  if (config.default_camera_control_enabled) {
    double init_target_dist = m_camera->position().norm();
    camera_target = m_camera->position() + m_camera->forward() * init_target_dist;
  }

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
    const Vec3 up {0, 1, 0};

    // Rotate around target, and translate in the view plane
    Vec3 rotate;
    Vec3 translate;
    for (auto& input : m_input_bus->get<CursorDrag>()) {
      auto mov = *(input.get<CursorDrag>());
      if (mov.alter == CursorAlterType::Left || mov.alter == CursorAlterType::None) {
        rotate += (mov.stop - mov.start);
      } else {
        translate += (mov.stop - mov.start);
      }
    }
    if (rotate.norm2() > 0.0001) {
      const double rot_range = pi;
      m_camera->rotate_position(camera_target, up, -rotate.x / m_viewer->width() * rot_range);
      m_camera->rotate_position(
        camera_target, -m_camera->right(), -rotate.y / m_viewer->height() * rot_range);
      m_camera->look_at(camera_target, up);
    }
    if (translate.norm2() > 0.0001) {
      constexpr double scale = 0.02;
      Vec3 scaled_trans = (-1 * scale) * translate;
      double target_dist = (m_camera->position() - camera_target).norm();
      m_camera->move(scaled_trans.x, scaled_trans.y, 0);
      camera_target = m_camera->position() + m_camera->forward() * target_dist;
    }

    // Zoom in-or-out
    double zoom_in = 0;
    for (auto& input : m_input_bus->get<Zoom>()) {
      auto zoom = *(input.get<Zoom>());
      zoom_in += zoom.delta;
    }
    if (std::abs(zoom_in) != 0) {
      const double ideal_dist = 8;
      const double block_dist = 4;
      double curr_dist = (m_camera->position() - camera_target).norm();
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
  // update camera
  m_pipeline->transform_viewport(m_viewport_h, *m_camera);
  // update model transform
  for (ModelPtr& m : m_scene->get_models()) {
    // TODO may be we need some transform mark-dirty mechanics?
    m_pipeline->update_model(m_scene_h, *m);
  }
  // render
  m_pipeline->render(m_viewport_h, m_scene_h);

  // m_renderer->prepare(*m_scene);
  // ScreenTransformHandle viewport = m_viewer->get_viewport();
  // m_renderer->update_viewport_transform(viewport, *m_camera);
  // auto culling_result = m_renderer->cull(viewport, *m_scene);
  // m_renderer->render(viewport, culling_result);
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
