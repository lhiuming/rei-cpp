#include <memory>

#include <app_utils/app.h>
#include <direct3d/d3d_renderer.h>

using std::make_unique;
using std::unique_ptr;
using namespace rei;

class RayTracingApp : public App {
  using Base = App;
  using Base::Base;

public:
  RayTracingApp(Config conf) : WinApp(conf, create_renderer()) { }

  static unique_ptr<Renderer> create_renderer() { 
    d3d::Renderer::Options opt = {};
    opt.enable_realtime_raytracing = true;
    opt.init_render_mode = d3d::RenderMode::RealtimeRaytracing;
    //opt.draw_debug_model = true;
    auto ret = make_unique<d3d::Renderer>(get_hinstance(), opt);
    return ret;
  }

private:
  void on_start() override;
  void on_update() override;
};

void RayTracingApp::on_start() {
  Base::on_start();

  auto cube = std::make_shared<Mesh>(Mesh::procudure_cube());
  auto plane = std::make_shared<Mesh>(Mesh::procudure_cube({4, 0.1, 4}));
  auto sphere = std::make_shared<Mesh>(Mesh::procudure_sphere_icosahedron(3));
  scene().add_model(Mat4::translate({0, -0.1, 0}), plane, L"plane");
  scene().add_model(Mat4::translate({1.4, 1.1, 0}), cube, L"cube");
  scene().add_model(Mat4::translate({-1.2, 1.1, 0}), sphere, L"sphere");
}

void RayTracingApp::on_update() {
  Base::on_update();
}

int main() {
  App::Config conf = {};
  conf.title = L"Real-Time Ray-Tracing Demo (REI Sample)";
  conf.width = 1080;
  conf.height = 480;
  conf.bg_color = Colors::ayanami_blue;
  auto app = RayTracingApp(conf);
  app.run();
  return 0;
}

