// Test color and z-buffer

#include <iostream>

#include <model.h>
#include <scene.h>
#include <camera.h>
#include <renderer.h>
#include <viewer.h>

using namespace std;
using namespace CEL;

int main()
{
  // Create the triangles
  Vertex v1({10, 0, 0}), v2({10, 10, 0}), v3({-10, 0, 0});
  Vertex v4({0, 10, 0}), v5({-10, 10, 0}), v6({0, -10, 0});
  vector<Vertex> va{v1, v2, v3, v4, v5, v6};
  Mesh mesh{std::move(va), {0, 1, 2, 3, 4, 5}};
  cout << "Mesh model set up." << endl;

  // Set up the scene
  StaticScene s;
  s.add_model(ModelPtr(new Mesh(mesh)), Mat4::I());
  cout << "Scene set up. " << endl;

  // Set up the camera
  Camera c({0, 0, 20}, {0, 0, -1});
  c.set_ratio(720.0 / 480.0);
  cout << "Camera set up." << endl;

  // Set up the Viewer and Renderer
  SoftRenderer r;  // no much setting necessary
  Viewer v(720, 480, "Three Triangle (testing color and z-buffer)");
  v.set_camera(&c);
  v.set_scene(&s);
  v.set_renderer(&r);
  cout << "Viewer and Renderer set up." << endl;

  // run
  v.run();

  cout << "Viewer stopped. Program ends." << endl;

  return 0;
}
