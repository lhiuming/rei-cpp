// Test the Viewer class

#include <iostream>

#include <renderer.h>
#include <scene.h>
#include <viewer.h>

using namespace std;
using namespace CEL;

int main()
{

  // construction
  Viewer v(360, 280, "test viewer");
  cout << "Successfully craeted a Viewer." << endl;

  // setting renderer and scene
  StaticScene s;
  Renderer r;
  v.set_scene(&s);
  v.set_renderer(&r);
  cout << "Succefully set scene and renderer. " << endl;

  // try to draw
  v.run();

  return 0;
}
