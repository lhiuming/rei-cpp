#include <memory>

#include <app_utils/app.h>
#include <console.h>
#include <direct3d/d3d_renderer.h>

using namespace std;
using namespace rei;

class HelloApp : public App {
  using Base = App;
  using Base::Base;
  void on_start() override;

public:
  HelloApp(App::Config config) : App(config) {
    camera().move(0, 3, 0);
    camera().look_at({0, 1, 0});
  }
};

void HelloApp::on_start() {
  Base::on_start();
  MeshPtr cube = std::make_shared<Mesh>(std::move(Mesh::procudure_cube()));
  scene().add_model(Mat4::translate({0, 1, 0}), cube, L"Big Cube");
}

int main() {
  App::Config conf = {};
  conf.title = L"Hello Cube (REI Sample)";
  conf.width = 1080;
  conf.height = 720;
  conf.bg_color = Colors::ayanami_blue;
  conf.render_mode = App::RenderMode::Rasterization;
  auto app = HelloApp(conf);

  app.run();

  return 0;
}
