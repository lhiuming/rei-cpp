// Test color and z-buffer

#ifdef USE_MSVC
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

#include <memory>
#include <cstdlib>
#include <ctime>

#include <camera.h>
#include <console.h>
#include <model.h>
#include <renderer.h>
#include <scene.h>
#include <color.h>

#include <app_utils/app.h>
#include <debug.h>

using namespace std;
using namespace rei;

int main() {
  using Vertex = typename Mesh::Vertex;

  // Create the triangles
  Vertex v0({10, 0, 0}, {0.f, 0.f, 1.f}, {0.9f, 0.6f, 0.1f, 0.0f});
  Vertex v1({8, 10, 0}, {0.f, 0.f, 1.f}, {0.9f, 0.1f, 0.5f, 1.0f});
  Vertex v2({-12, 0, 2}, {0.2f, 0.f, .8f}, {0.8f, 0.4f, 0.1f, 1.0f});
  Vertex v3({0, 7, 0}, {0.f, 0.f, 1.f}, {0.4f, 0.8f, 0.0f, 0.0f});
  Vertex v4({-8, 6, 0}, {0.f, 0.f, 1.f}, {0.7f, 0.8f, 0.1f, 1.0f});
  Vertex v5({0, -13, 2}, {0.f, 0.2f, .8f}, {0.1f, 0.8f, 0.6f, 1.0f});
  Vertex v6({5, -9, 0}, {0.f, 0.f, 1.f}, {0.4f, 0.1f, 0.7f, 1.0f});
  Vertex v7({8, 4, 2}, {0.f, 0.f, 1.f}, {0.0f, 0.7f, 0.7f, 1.0f});
  Vertex v8({-6, -6, 0}, {0.f, 0.f, 1.f}, {0.2f, 0.2f, 0.7f, 0.0f});
  MeshPtr mesh = make_shared<Mesh>();
  mesh->set({v0, v1, v2, v3, v4, v5, v6, v7, v8}, {0, 1, 2, 3, 4, 5, 6, 7, 8});

  // Set up the scene
  Scene scene {};
  scene.add_model(Mat4::I(), mesh, L"triangles");

  // Set up the camera
  Camera camera (Vec3 {0.0, 2.0, 20.0});
  camera.set_aspect(720.0 / 480.0);
  console << "Camera set up." << endl;

  Color colors[4] = {Colors::jo, Colors::ha, Colors::kyu, Colors::final};
  std::srand(std::time(nullptr));
  Color rand_color = colors[std::rand() % 4];

  App::Config conf;
  conf.title = L"Three Triangles!";
  conf.width = 720;
  conf.height = 480;
  conf.bg_color = rand_color;
  auto app = App(conf);
  app.setup(std::move(scene), std::move(camera));
  app.run();

  console << "Viewer stopped. Program ends." << endl;

  return 0;
}
