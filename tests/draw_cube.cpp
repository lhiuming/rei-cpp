// Test loading and drwing mesh

#include <iostream>

#include <model.h>
#include <asset_loader.h>
#include <scene.h>
#include <camera.h>

#include <renderer.h>
#include <viewer.h>

using namespace std;
using namespace CEL;

int main(int argc, char** argv)
{
  // Read the .dae model
  string fn;
  if (argc > 1) {
    fn = string(argv[1]);
  } else {
    fn = string("../tests/color_cube.dae");
    cout << "using default input file: " << fn << endl;
  }

  AssetLoader loader;
  auto meshes = loader.load_mesh(fn);
  cout << "Model read. Get " << meshes.size() << " meshes. " << endl;

  // Check the model
  for(const auto& t : meshes[0]->get_triangles())
  {
    cout << "triangle: " << endl;
    cout << "  " << t.a->coord << ", " << t.a->color << endl;
    cout << "  " << t.b->coord << ", " << t.b->color << endl;
    cout << "  " << t.c->coord << ", " << t.c->color << endl;
  }
  // Set up the scene
  auto s = make_shared<StaticScene>();
  s->add_model(meshes[0], Mat4::I()); // push it as it-is
  cout << "Scene set up. " << endl;

  // window size
  const int width = 1280;
  const int height = 720;

  // Set up the camera
  auto c = make_shared<Camera>(Vec3{0, 0, 20}, Vec3{0, 0, -1});
  c->set_ratio((float)width / height);
  cout << "Camera set up." << endl;

  // Set up the Viewer and Renderer
  auto v = makeViewer(width, height,
    "Three Triangle (testing color and z-buffer)");
  auto r = makeRenderer(); // must put after Viewer construction !
  v->set_camera(c);
  v->set_scene(s);
  v->set_renderer(r);
  cout << "Viewer and Renderer set up." << endl;

  // run
  v->run();

  cout << "Viewer stopped. Program ends." << endl;

  return 0;
}
