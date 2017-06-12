// Test model module

#include <console.h>
#include <model.h>

using namespace std;
using namespace CEL;

int main()
{
  // set vertices
  Mesh::Vertex a(Vec3(0, 0, 0)), b(Vec3(1, 0, 0)), c(Vec3(1, 1, 0));
  vector<Mesh::Vertex> vao({a, b, c});

  // set triangle
  vector<Mesh::size_type> vbo{0, 1, 2};

  // make the mesh
  Mesh mesh;
  mesh.set(std::move(vao), vbo);

  console << "Created a Mesh" << endl;

  const auto& vertices = mesh.get_vertices();
  for (const auto& tri : mesh.get_triangles())
  {
    console << vertices[tri.a].coord << endl;
    console << vertices[tri.b].coord << endl;
    console << vertices[tri.c].coord << endl;
  }

  return 0;
}
