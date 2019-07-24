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
  RayTracingApp(Config conf) : WinApp(conf) { }

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
  conf.render_mode = App::RenderMode::RealtimeRaytracing;
  auto app = RayTracingApp(conf);
  app.run();
  return 0;
}

