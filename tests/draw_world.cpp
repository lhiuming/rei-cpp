// Test loading and drwing a 3D file as a world

#ifdef USE_MSVC
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

#include <model.h>
#include <scene.h>
#include <camera.h>
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

  // window size
  const int width = 720;
  const int height = 480;



  return 0;
}
