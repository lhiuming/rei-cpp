// Test loading and drwing a 3D file as a world

#ifdef USE_MSVC
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

#include <scene.h>
#include <asset_loader.h>
#include <renderer.h>
#include <viewer.h>
#include <console.h>


using namespace std;
using namespace CEL;

int main(int argc, char** argv)
{
  // Read the .dae model
  string fn;
  if (argc > 1) {
    fn = string(argv[1]);
  } else {
    console << "No input file!" << fn << endl;
    return -1;
  }

  // Load the world elements
  AssetLoader loader;
  auto world_element = loader.load_world(fn);
  ScenePtr s = get<0>(world_element);
  //CameraPtr c = get<1>(world_element);  // Assimp camera is not correct
  CameraPtr c = make_shared<Camera>(Vec3(0.0, 0.0, 10));
  console << "World loaded." << endl;

  // check the things
  console << "-- Check the loaded world --" << endl;
  console << "Summary of the Scene: \n    " << *s;
  console << "Summary of Camera: \n    " << *c;

  // Make a Renderer
  auto r = makeRenderer();

  // Set up the Viewer and Renderer
  const int width = 480;
  int height = width / c->get_aspect();
  auto v = makeViewer(width, height, "Draw World from file");
  v->set_camera(c);
  v->set_scene(s);
  v->set_renderer(r);
  console << "Viewer and Renderer set up." << endl;

  // run
  v->run();

  console << "Program closing." << endl;

  return 0;


  return 0;
}
