// Test the loader lib

#include <asset_loader.h>
#include <console.h>

#include <iostream>
#include <string>

using namespace std;
using namespace CEL;

void check_mesh(const Mesh& mesh);
void check_scene(const Scene& s);
void check_camera(const Camera& c);
void check_light(const Light* l);

int main(int argc, char** argv)
{
  string fn;
  if (argc > 1) {
    fn = string(argv[1]);
  } else {
    console << "No input file" << endl;
    return -1;
  }

  // settings
  bool verbose = false;
  bool load_world = false;
  for (int i = 2; i < argc; ++i) {
    if (string(argv[i]) == "-w") load_world = true;
    else if (string(argv[i]) == "-v") verbose = true;
  }

  AssetLoader loader;
  if (load_world)
  {
    auto world_stuffs = loader.load_world(fn);
    console << "Load world stuffs." << endl;
    if (verbose) {
      // print the scenes, camera and light
      check_scene( *(get<0>(world_stuffs)) );
    }
  }
  else  // load meshed
  {
    auto meshes = loader.load_meshes(fn);
    console << "Get " << meshes.size() << " meshes. " << endl;
    if (verbose) {
      // have a look on the mesh
      check_mesh( *(meshes[0]) );
    }
  }

  return 0;
}


void check_scene(const Scene& s)
{
  return;
}



void check_mesh(const Mesh& mesh)
{
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

  return;
}
