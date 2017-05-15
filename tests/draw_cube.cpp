// Test loading and drwing mesh

#include <iostream>

#include <model.h>
#include <scene.h>
#include <camera.h>
#include <renderer.h>
#include <viewer.h>
#include <asset_loader.h>

using namespace std;
using namespace CEL;

int main(int argc, char** argv)
{
  // Read the .dae model
  string fn;
  if (argc > 1) {
    fn = string(argv[1]);
  } else {
    fn = string("../tests/cube.dae");
    cout << "using default input file: " << fn << endl;
  }

  AssetLoader loader;
  auto meshes = loader.load_mesh(fn);
  cout << "Model read. Get " << meshes.size() << " meshes. " << endl;

  // Set up the scene
  StaticScene s;
  s.add_model(meshes[0], Mat4::I()); // push it as it-is
  cout << "Scene set up. " << endl;

  // window size
  const int width = 1280;
  const int height = 720;

  // Set up the camera
  Camera c({0, 0, 20}, {0, 0, -1});
  c.set_ratio((float)width / height);
  cout << "Camera set up." << endl;

  // Set up the Viewer and Renderer
  SoftRenderer r;  // no much setting necessary
  Viewer v(width, height, "Three Triangle (testing color and z-buffer)");
  v.set_camera(&c);
  v.set_scene(&s);
  v.set_renderer(&r);
  cout << "Viewer and Renderer set up." << endl;

  // run
  v.run();

  cout << "Viewer stopped. Program ends." << endl;

  return 0;
}
