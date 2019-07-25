#include <memory>

#include <scene.h>

#include <app_utils/app.h>
#include <direct3d/d3d_renderer.h>

using namespace std;
using namespace rei;

class RayTracingApp : public App {
  using Base = App;
  using Base::Base;

public:
  RayTracingApp(Config conf) : WinApp(conf) {}

private:
  void on_start() override;
  void on_update() override;
};

void RayTracingApp::on_start() {
  Base::on_start();

  auto blue_steel = make_shared<Material>(L"Blue Steel");
  {
    blue_steel->set(L"albedo", Colors::aqua);
    blue_steel->set(L"metalness", 1.0);
    blue_steel->set(L"smoothness", 0.9);
  }
  auto yellow_plastic = make_shared<Material>(L"Yellow Plastic");
  {
    yellow_plastic->set(L"albedo", Colors::yellow);
    yellow_plastic->set(L"metalness", 0.0);
    yellow_plastic->set(L"smoothness", 0.7);
  }
  auto dark_wood = make_shared<Material>(L"Dark Wood");
  {
    dark_wood->set(L"albedo", Colors::asuka_red * 0.5f);
    dark_wood->set(L"metalness", 0.0);
    dark_wood->set(L"smoothness", 0.1);
  }

  auto cube = std::make_shared<Mesh>(Mesh::procudure_cube());
  auto plane = std::make_shared<Mesh>(Mesh::procudure_cube({4, 0.1, 4}));
  auto sphere = std::make_shared<Mesh>(Mesh::procudure_sphere_icosahedron(3));
  scene().add_model(Mat4::translate({0, -0.1, 0}), plane, blue_steel, L"plane");
  scene().add_model(Mat4::translate({1.4, 1.1, 0}), cube, dark_wood, L"cube");
  scene().add_model(Mat4::translate({-1.2, 1.1, 0}), sphere, yellow_plastic, L"sphere");
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
  conf.render_mode = App::RenderMode::Hybrid;
  auto app = RayTracingApp(conf);
  app.run();
  return 0;
}
