// Test the loader lib

#include <asset_loader.h>

#include <iostream>
#include <string>

using namespace std;
using namespace CEL;

int main(int argc, char** argv)
{
  string fn;
  if (argc > 1) {
    fn = string(argv[1]);
  } else {
    fn = string("../tests/cube.dae");
    cout << "using default input file: " << fn << endl;
  }

  AssetLoader loader;

  auto meshes = loader.load_mesh(fn);

  cout << "Get " << meshes.size() << " meshes. " << endl;

  // have a look on the mesh
  const Mesh& mesh = *(meshes[0]);
  for (const auto& t : mesh.get_triangles())
  {
    cout << "Triange: " << endl;
    cout << "  " << t.a->coord << endl;
    cout << "  " << t.b->coord << endl;
    cout << "  " << t.c->coord << endl;
  }

  return 0;
}
