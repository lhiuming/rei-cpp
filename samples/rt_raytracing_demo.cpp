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
    auto ret = make_unique<d3d::Renderer>(get_hinstance(), opt);
    return ret;
  }

private:
  void on_start() override;
  void on_update() override;
};

void RayTracingApp::on_start() {
  Base::on_start();
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

