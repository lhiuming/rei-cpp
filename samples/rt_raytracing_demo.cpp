#include <memory>

#include <math/num_types.h>
#include <scene.h>

#include <app_utils/app.h>

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

  // quick switch to remove all emission
  const double emit_inten = 1.0;

  auto silver = create_material(L"Smooth Silver");
  {
    silver->set(L"albedo", Colors::fresnel0::silver);
    silver->set(L"metalness", 1.0);
    silver->set(L"smoothness", 0.85);
  }
  auto stone = create_material(L"Light Stone");
  {
    stone->set(L"albedo", Colors::white * 0.5);
    stone->set(L"metalness", 0.0);
    stone->set(L"smoothness", 0.1);
  }
  auto yellow_plastic = create_material(L"Yellow Plastic");
  {
    yellow_plastic->set(L"albedo", Colors::yellow);
    yellow_plastic->set(L"metalness", 0.0);
    yellow_plastic->set(L"smoothness", 0.5);
  }
  auto dark_wood = create_material(L"Dark Wood");
  {
    dark_wood->set(L"albedo", Colors::asuka_red * 0.3);
    dark_wood->set(L"metalness", 0.0);
  }
  auto weak_light = create_material(L"Weak Light");
  {
    weak_light->set(L"albedo", Colors::white);
    weak_light->set(L"emissive", 0.5 / pi * emit_inten);
  }
  auto mild_light = create_material(L"Mild Light");
  {
    mild_light->set(L"albedo", Colors::white);
    mild_light->set(L"emissive", 1 / pi * emit_inten);
  }
  auto super_light = create_material(L"Super-Bright Light");
  {
    super_light->set(L"albedo", Color(1.f, .7f, .7f));
    super_light->set(L"emissive", 10.0 * emit_inten);
  }

  auto cube = create_geometry(Mesh::procudure_cube());
  auto tube = create_geometry(Mesh::procudure_cube({1, .1, .1}));
  auto plane = create_geometry(Mesh::procudure_cube({4, 0.1, 4}));
  auto half_plane = create_geometry(Mesh::procudure_cube({2, 0.1, 4}));
  auto small_plane = create_geometry(Mesh::procudure_cube({4, 0.1, 2}));
  auto sphere = create_geometry(Mesh::procudure_sphere_icosahedron(3));
  auto dot = create_geometry(Mesh::procudure_sphere_icosahedron(3, 0.25f));
  // create_model(Mat4::translate({0, -0.1, 0}), plane, blue_steel, L"plane");
  create_model(Mat4::translate({0, -0.1, 0}), plane, stone, L"wood plane");
  create_model(Mat4::translate_rotate({0, 2.2, -4}, {1, 0, 0}, 90 * degree), small_plane, silver,
    L"metal plane");
  create_model(Mat4::translate({1.5, 1.0, 0}), cube, dark_wood, L"cube");
  create_model(Mat4::translate({-1, 1.0, 0}), sphere, yellow_plastic, L"sphere");
  create_model(Mat4::translate({0, 1, -3}), sphere, weak_light, L"light sphere");
  // small dot-like lights
  create_model(Mat4::translate({1.5, 2.5, 0}), dot, super_light, L"light blob");
  // Reference ball list
  auto small_ball = create_geometry(Mesh::procudure_sphere_icosahedron(3, 0.25));
  const int balls_x = 8;
  const int balls_y = 2;
  for (int i = 0; i < balls_x; i++) {
    for (int j = 0; j < balls_y; j++) {
      Vec3 pos = {(i - balls_x / 2) * 0.6f + j * 0.3f, 0.25f, 2.4f + j * 0.6f};
      auto ball_mat = create_material(L"Varying Ball");
      ball_mat->set(L"albedo", Colors::fresnel0::copper);
      ball_mat->set(L"smoothness", i / double(balls_x));
      ball_mat->set(L"metalness", j / (std::max)(double(balls_y - 1.0), 1.0));
      create_model(Mat4::translate(pos), small_ball, ball_mat, L"One of Small Balls");
    }
  }
  // tube lights
  auto tube_arrange = [&](int i) {
    static const Mat4 center_up = Mat4::translate({0, 1, 0});
    static const Mat4 side_way = Mat4::translate({0, 0, 3.2});
    return Mat4::rotate({0, 1, 0}, (2 * pi) * (i + 2.5) / 5) * side_way * center_up;
  };
  create_model(tube_arrange(1), tube, mild_light, L"tube light");
  create_model(tube_arrange(2), tube, mild_light, L"tube light");
  create_model(tube_arrange(3), tube, mild_light, L"tube light");
  create_model(tube_arrange(4), tube, mild_light, L"tube light");
}

void RayTracingApp::on_update() {
  Base::on_update();
}

int main() {
  App::Config conf = {};
  conf.title = L"Real-Time Ray-Tracing Demo (REI Sample)";
  conf.width = 1920;
  conf.height = 1080;
  conf.bg_color = Colors::ayanami_blue;
  conf.render_mode = App::RenderMode::Hybrid;
  auto app = RayTracingApp(conf);
  app.run();
  return 0;
}
