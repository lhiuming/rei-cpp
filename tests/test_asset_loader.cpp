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

  auto meshes = loader.load_meshes(fn);

  console << "Get " << meshes.size() << " meshes. " << endl;

  // have a look on the mesh
  const Mesh& mesh = *(meshes[0]);
  // vertices and triangles
  const auto& vert = mesh.get_vertices();
  for (const auto& t : mesh.get_triangles())
  {
    console << "Triangle: " << endl;
    console << "coordinate: " << endl;
    console << "  " << vert[t.a].coord << endl;
    console << "  " << vert[t.b].coord << endl;
    console << "  " << vert[t.c].coord << endl;
    console << "normal: " << endl;
    console << "  " << vert[t.a].normal << endl;
    console << "  " << vert[t.b].normal << endl;
    console << "  " << vert[t.c].normal << endl;
    console << "color: " << endl;
    console << "  " << vert[t.a].color << endl;
    console << "  " << vert[t.b].color << endl;
    console << "  " << vert[t.c].color << endl;
  }
  // materials
  auto& mat = mesh.get_material();
  console << "Material: " << endl;
  console << "diffuse: " << mat.diffuse << endl;
  console << "ambient: " << mat.ambient << endl;
  console << "specular:" << mat.specular << endl;
  console << "shine:   " << mat.shineness << endl;

  return 0;
}
