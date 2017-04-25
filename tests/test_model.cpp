// Test model module

#include <iostream>
#include <model.h>

using namespace std;
using namespace CEL;

int main()
{
  Vertex a(Vec3(0, 0, 0)), b(Vec3(1, 0, 0)), c(Vec3(1, 1, 0));
  vector<Vertex> vao({a, b, c});
  Triangle<NaiveMesh::VertexCIt> t(vao.cbegin(), vao.cbegin() + 1, vao.cbegin() + 2);
  vector<Triangle<NaiveMesh::VertexCIt>> vbo{t};

  NaiveMesh mesh(vao, vbo);

  cout << "Created a NaiveMesh" << endl;

  for (auto tri = mesh.triangles_cbegin(); tri != mesh.triangles_cend(); ++tri)
  {
    cout << tri->a->pos << endl;
    cout << tri->b->pos << endl;
    cout << tri->c->pos << endl;
  }

  return 0;
}
