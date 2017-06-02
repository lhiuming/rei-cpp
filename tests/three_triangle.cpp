// Test color and z-buffer

#ifdef USE_MSVC
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

#include <memory>

#include <model.h>
#include <scene.h>
#include <camera.h>
#include <renderer.h>
#include <viewer.h>
#include <console.h>

using namespace std;
using namespace CEL;

int main()
{
  using Vertex = typename Mesh::Vertex;

  // Create the triangles
  Vertex v1({10, 0, 0}), v2({8, 7, 0}), v3({-5, 0, 0});
  Vertex v4({0, 6, 0}), v5({-3, 0, 0}), v6({0, -10, 0});
  Vertex v7({0.8f, 0.0f, 0.5}), v8({0.8f, 0.8f, 0.5f}), v9({0.0f, 0.8f, 0.5f});
  Mesh mesh{{v1, v2, v3, v4, v5, v6, v7, v8, v9}, 
            {0, 1, 2,  3, 4, 5,  6, 7, 8}};
  console << "Mesh model set up." << endl;

  // Set up the scene
  auto s = make_shared<StaticScene>();
  s->add_model(make_shared<Mesh>(std::move(mesh)), Mat4::I());
  console << "Scene set up. " << endl;

  // Set up the camera
  auto c = make_shared<Camera>(Vec3{0.0, 2.0, 20.0});
  c->set_ratio(720.0 / 480.0);
  console << "Camera set up." << endl;

  // Create a renderer 
  auto r = makeRenderer();  // no much setting necessary

  // Set up the Viewer and Renderer
  const int width = 720, height = 480;
  auto viewer = makeViewer(width, height, "Three Triangles");
  viewer->set_camera(c);
  viewer->set_scene(s);
  viewer->set_renderer(r);
  console << "Viewer and Renderer set up." << endl;

  // run
  viewer->run();

  console << "Viewer stopped. Program ends." << endl;

  return 0;
}
