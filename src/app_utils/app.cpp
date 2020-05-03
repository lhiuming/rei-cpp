#include "app.h"

#include <iomanip>
#include <sstream>

#include "../debug.h"
#include "../renderer.h"
#include "editor/imgui_global.h"
#include "math/math_utils.h"

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

  // Default viewer
  m_viewer = make_unique<WinViewer>(hinstance, config.width, config.height, config.title);
  m_viewer->set_input_bus(m_input_bus);
  // m_renderer->set_viewport_clear_value(m_viewer->get_viewport(), config.bg_color);

  // Create default scene and camera
  m_geometries = make_shared<Geometries>();
  m_materials = make_shared<Materials>();
  m_scene = make_shared<Scene>();
  m_camera = make_shared<Camera>(Vec3 {0, 0, 10}, Vec3 {0, 0, -1});
  m_camera->set_aspect(config.width, config.height);

  // Default renderer & pipeline
  {
    Renderer::Options r_opts = {};
    m_renderer = make_shared<Renderer>(hinstance, r_opts);
  }
  switch (config.render_mode) {
    case RenderMode::Rasterization:
    case RenderMode::RealtimeRaytracing:
      // m_pipeline = make_shared<RealtimePathTracingPipeline>(d3d_renderer);
      REI_ERROR("Support for this render pipeline is dropped.");
      break;
    case RenderMode::Hybrid:
    default: {
      HybridPipeline::Context ctx;
      ctx.renderer = m_renderer;
      ctx.wnd_id = m_viewer->get_window_id();
      ctx.scene = m_scene;
      ctx.camera = m_camera;
      m_pipeline = make_shared<HybridPipeline>(ctx);
      break;
    }
  }
}

WinApp::~WinApp() {
  log("WinApp terminated.");
}

void WinApp::initialize_scene() {}

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

void WinApp::begin_tick() {
  // NOTE: must set up correct display size before begin new frame
  g_ImGUI.set_display_size(float(m_viewer->width()), float(m_viewer->height()));
  g_ImGUI.begin_new_frame();
}

void WinApp::end_tick() {}

void WinApp::update() {
  // callback
  on_update();

  // Finalize
  m_input_bus->reset();
}

void WinApp::on_update() {
  update_ui();
  update_camera_control();
  update_title();
}

void WinApp::update_ui() {
  // Update dev UI
  if (!m_input_bus->empty<CursorMove>()) {
    Vec3 cursor_pos;
    for (const auto& move : m_input_bus->get<CursorMove>()) {
      cursor_pos = move.stop;
    }
    // NOTE: ImGUI use top-left base
    float pos_x = m_input_bus->cursor_left_top().x + cursor_pos.x;
    float pos_y = m_input_bus->cursor_right_bottom().y - cursor_pos.y;
    // TODO remove this after viewport resizing is implemented
    float vp_scale_x = 1920.0f / m_viewer->width();
    float vp_scale_y = 1080.0f / m_viewer->height();
    g_ImGUI.update_mouse_pos(pos_x * vp_scale_x, pos_y * vp_scale_y);
  }
  g_ImGUI.update_mouse_down(0, !m_input_bus->empty<CursorDown>());
  g_ImGUI.update_mouse_clicked(0, !m_input_bus->empty<CursorUp>());

  if (config.enable_dev_gui) {
    // Toggle dev ui
    m_input_bus->get<KeyUp>().for_each([&](const KeyUp& keyup) {
      switch (keyup.key) {
        case KeyCode::F1:
          m_show_dev_ui = !m_show_dev_ui;
          break;
        case KeyCode::F2:
          m_show_dev_ui_demo = !m_show_dev_ui_demo;
          m_show_dev_ui &= !m_show_dev_ui_demo;
          break;
      }
    });

    if (m_show_dev_ui_demo) { g_ImGUI.show_imgui_demo(); }

    if (m_show_dev_ui) {
      // on_gui events
      m_pipeline->on_gui();
    }
  }
}

void WinApp::update_camera_control() {
  if (config.default_camera_control_enabled) {
    const Vec3 up {0, 1, 0};
    // Rotate around target, and translate in the view plane
    Vec3 rotate;
    Vec3 translate;
    for (auto& mov : m_input_bus->get<CursorDrag>()) {
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
    for (auto& zoom : m_input_bus->get<Zoom>()) {
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
  // main thread rendering
  size_t width = m_viewer->width();
  size_t height = m_viewer->height();
  m_pipeline->render(width, height);
}

GeometryPtr WinApp::create_geometry(Mesh&& mesh) {
  auto ptr = m_geometries->create(L"unamed_mesh", std::move(mesh));
  m_pipeline->on_create_geometry(*ptr);
  return ptr;
}

MaterialPtr WinApp::create_material(Name&& name) {
  auto ptr = m_materials->create(std::move(name));
  m_pipeline->on_create_material(*ptr);
  return ptr;
}

ModelPtr WinApp::create_model(const Mat4& trans, GeometryPtr geo, MaterialPtr mat, Name&& name) {
  auto ptr = m_scene->create_model(trans, geo, mat, std::move(name));
  m_pipeline->on_create_model(*ptr);
  return ptr;
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
    begin_tick();
    update();
    render();
    end_tick();
  } // end while
}

} // namespace rei
