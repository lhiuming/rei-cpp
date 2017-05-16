// Test the Viewer class

#include <iostream>
#include <memory>

#include <renderer.h>
#include <scene.h>
#include <viewer.h>

using namespace std;
using namespace CEL;

int main()
{

  // construction
  auto pv = makeViewer(360, 280, "test viewer");
  Viewer& v = *pv;
  cout << "Successfully craeted a Viewer." << endl;

  // setting renderer and scene
  auto c = make_shared<Camera>();
  auto s = make_shared<StaticScene>();
  auto r = makeRenderer();
  v.set_camera(c);
  v.set_scene(s);
  v.set_renderer(r);
  cout << "Succefully set camera, scene, and renderer. " << endl;

  // try to draw
  v.run();

  cout << "Viewer stop running." << endl;

  return 0;
}
