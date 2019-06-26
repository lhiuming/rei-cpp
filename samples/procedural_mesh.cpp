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

  MeshPtr cube;
};

void ProceduralApp::on_start() {
  MeshPtr cube = std::make_shared<Mesh>(std::move(Mesh::procudure_cube()));
  scene->add_model(Mat4::I(), cube, L"Big Cube");
}

void ProceduralApp::on_update() {
  Base::on_update();

  Scene::ModelsRef models = scene->get_models();
  static Mat4 rotations[1] = {Mat4 {{}, {0, 1, 0}, 0.0005},};
  for (ModelPtr& m : models) {
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
