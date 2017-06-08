// Test the loader lib

#include <asset_loader.h>
#include <console.h>

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
    fn = string("color_cube.dae");
    console << "using default input file: " << fn << endl;
  }

  AssetLoader loader;

  auto meshes = loader.load_mesh(fn);

  console << "Get " << meshes.size() << " meshes. " << endl;

  // have a look on the mesh
  const Mesh& mesh = *(meshes[0]);
  for (const auto& t : mesh.get_triangles())
  {
    console << "Triange: " << endl;
    console << "coordinate: " << endl;
    console << "  " << t.a->coord << endl;
    console << "  " << t.b->coord << endl;
    console << "  " << t.c->coord << endl;
    console << "normal: " << endl;
    console << "  " << t.a->normal << endl;
    console << "  " << t.b->normal << endl;
    console << "  " << t.c->normal << endl;
    console << "color: " << endl;
    console << "  " << t.a->color << endl;
    console << "  " << t.b->color << endl;
    console << "  " << t.c->color << endl;
  }

  return 0;
}
