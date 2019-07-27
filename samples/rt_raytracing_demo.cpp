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
    blue_steel->set(L"smoothness", 0.95);
  }
  auto yellow_plastic = make_shared<Material>(L"Yellow Plastic");
  {
    yellow_plastic->set(L"albedo", Colors::yellow);
    yellow_plastic->set(L"metalness", 0.0);
    yellow_plastic->set(L"smoothness", 0.5);
    yellow_plastic->set(L"emissive", 0.2);
  }
  auto dark_wood = make_shared<Material>(L"Dark Wood");
  {
    dark_wood->set(L"albedo", Colors::asuka_red * 0.3);
    dark_wood->set(L"metalness", 0.0);
    dark_wood->set(L"smoothness", 0.0);
  }
  auto bright_light = make_shared<Material>(L"Bright Light");
  {
    bright_light->set(L"albedo", Colors::white * 0.1);
    bright_light->set(L"emissive", 10);
  }
  auto super_light = make_shared<Material>(L"Super-Bright Light");
  {
    super_light->set(L"albedo", Colors::white);
    super_light->set(L"emissive", 50);
  }

  auto cube = std::make_shared<Mesh>(Mesh::procudure_cube());
  auto plane = std::make_shared<Mesh>(Mesh::procudure_cube({4, 0.1, 4}));
  auto sphere = std::make_shared<Mesh>(Mesh::procudure_sphere_icosahedron(3));
  auto dot = std::make_shared<Mesh>(Mesh::procudure_sphere_icosahedron(3, 0.25f));
  scene().add_model(Mat4::translate({0, -0.1, 0}), plane, blue_steel, L"plane");
  scene().add_model(Mat4::translate({1.4, 1.1, 0}), cube, dark_wood, L"cube");
  scene().add_model(Mat4::translate({-1.2, 1.1, 0}), sphere, yellow_plastic, L"sphere");
  scene().add_model(Mat4::translate({0, 1, -3}), sphere, bright_light, L"light sphere");
  scene().add_model(Mat4::translate({3, 2, 2}), dot, super_light, L"light blob");
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
