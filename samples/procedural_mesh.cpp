// Test color and z-buffer

#ifdef USE_MSVC
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

#include <memory>

#include <app_utils/app.h>
#include <camera.h>
#include <console.h>
#include <geometry.h>
#include <scene.h>

using namespace std;
using namespace rei;

class ProceduralApp : public App {
  using Base = App;
  using Base::Base;
  void on_start() override;
  void on_update() override;

  int rotating_cube_index = -1;
};

void ProceduralApp::on_start() {
  MeshPtr cube = std::make_shared<Mesh>(std::move(Mesh::procudure_cube()));
  MeshPtr pillar = std::make_shared<Mesh>(std::move(Mesh::procudure_cube({0.5, 4, 0.5})));
  MeshPtr sphere_bad = std::make_shared<Mesh>(std::move(Mesh::procudure_sphere(0)));
  MeshPtr sphere_good  = std::make_shared<Mesh>(std::move(Mesh::procudure_sphere(2)));
  MeshPtr sphere_ultra = std::make_shared<Mesh>(std::move(Mesh::procudure_sphere(4)));
  MeshPtr plane = std::make_shared<Mesh>(std::move(Mesh::procudure_cube({4, 0.125, 4})));
  scene().add_model(Mat4::translate({0, 1, 0}) * Mat4::from_diag({0.3, 0.3, 0.3, 1.0}), cube, L"Small Cube");
  rotating_cube_index = 0;
  scene().add_model(Mat4::translate({0, 1, 1}), sphere_good, L"Sphere");
  scene().add_model(Mat4::translate({-2, 1, 1}), sphere_bad, L"Sphere");
  scene().add_model(Mat4::translate({2, 1, 1}), sphere_ultra, L"Sphere");
  scene().add_model(Mat4::translate({-2.5, 1, -2}), cube, L"Big Cube");
  scene().add_model(Mat4::translate({3, 4, -2}), pillar, L"Tall Pillar");
  scene().add_model(Mat4::translate({0, -0.125, 0}), plane, L"Lager Plane");

  camera().move(0, 3, 0);
  camera().look_at({0, 1, 0});
}

void ProceduralApp::on_update() {
  Base::on_update();

  Scene::ModelsRef models = scene().get_models();
  static Mat4 rotations[1] = {
    Mat4::translate_rotate({}, {0, 1, 0}, 0.0005),
  };
  if (rotating_cube_index > 0)
  { // rotating the small cube
    ModelPtr& m = models[rotating_cube_index];
    Mat4 mat = m->get_transform();
    mat = mat * rotations[0];
    m->set_transform(mat);
  }
}

int main() {
  App::Config conf = {};
  conf.title = L"Procedural Mesh (REI Sample)";
  conf.width = 1080;
  conf.height = 480;
  conf.bg_color = Colors::ayanami_blue;
  auto app = ProceduralApp(conf);

  app.run();

  return 0;
}
