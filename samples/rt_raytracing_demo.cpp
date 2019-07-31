#include <memory>

#include <rmath.h>
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
    blue_steel->set(L"smoothness", 0.85);
  }
  auto yellow_plastic = make_shared<Material>(L"Yellow Plastic");
  {
    yellow_plastic->set(L"albedo", Colors::yellow);
    yellow_plastic->set(L"metalness", 0.0);
    yellow_plastic->set(L"smoothness", 0.5);
  }
  auto dark_wood = make_shared<Material>(L"Dark Wood");
  {
    dark_wood->set(L"albedo", Colors::asuka_red * 0.3);
    dark_wood->set(L"metalness", 0.0);
  }
  auto weak_light = make_shared<Material>(L"Weak Light");
  {
    weak_light->set(L"albedo", Colors::white);
    weak_light->set(L"emissive", 0.75 / pi);
  }
  auto mild_light = make_shared<Material>(L"Mild Light");
  {
    mild_light->set(L"albedo", Colors::white);
    mild_light->set(L"emissive", 2 / pi);
  }
  auto super_light = make_shared<Material>(L"Super-Bright Light");
  {
    super_light->set(L"albedo", Color(1.f, .7f, .7f));
    super_light->set(L"emissive", 10.0);
  }

  auto cube = std::make_shared<Mesh>(Mesh::procudure_cube());
  auto tube = std::make_shared<Mesh>(Mesh::procudure_cube({1, .1, .1}));
  auto plane = std::make_shared<Mesh>(Mesh::procudure_cube({4, 0.1, 4}));
  auto sphere = std::make_shared<Mesh>(Mesh::procudure_sphere_icosahedron(3));
  auto dot = std::make_shared<Mesh>(Mesh::procudure_sphere_icosahedron(3, 0.25f));
  scene().add_model(Mat4::translate({0, -0.1, 0}), plane, blue_steel, L"plane");
  scene().add_model(Mat4::translate({1.5, 1.0, 0}), cube, dark_wood, L"cube");
  scene().add_model(Mat4::translate({-1, 1.0, 0}), sphere, yellow_plastic, L"sphere");
  scene().add_model(Mat4::translate({0, 1, -3}), sphere, weak_light, L"light sphere");
  // small dot-like lights
  scene().add_model(Mat4::translate({1.5, 2.5, 0}), dot, super_light, L"light blob");
  // Reference ball list
  auto small_ball = std::make_shared<Mesh>(Mesh::procudure_sphere_icosahedron(3, 0.25));
  const int balls_x = 8;
  const int balls_y = 2;
  for (int i = 0; i < balls_x; i++) {
    for (int j = 0; j < balls_y; j++) {
      Vec3 pos = {(i - balls_x / 2) * 0.6 + j * 0.3, 0.25, 2.4 + j * 0.6};
      auto ball_mat = make_shared<Material>(L"Varying Ball");
      ball_mat->set(L"albedo", Colors::white * 0.5);
      ball_mat->set(L"smoothness", i / double(balls_x));
      ball_mat->set(L"metalness", j / (std::max)(double(balls_y - 1), 1.0));
      scene().add_model(Mat4::translate(pos), small_ball, ball_mat, L"One of Small Balls");
    }
  }
  // tube lights
  auto tube_arrange = [&](int i) {
    static const Mat4 center_up = Mat4::translate({0, 1, 0});
    static const Mat4 side_way = Mat4::translate({0, 0, 3.2});
    return  Mat4::rotate({0, 1, 0}, (2 * pi) * (i + 2.5) / 5) * side_way * center_up;
  };
  scene().add_model(tube_arrange(1), tube, mild_light, L"tube light");
  scene().add_model(tube_arrange(2), tube, mild_light, L"tube light");
  scene().add_model(tube_arrange(3), tube, mild_light, L"tube light");
  scene().add_model(tube_arrange(4), tube, mild_light, L"tube light");
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
