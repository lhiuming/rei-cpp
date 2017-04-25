// Test model module

#include <iostream>
#include <model.h>

using namespace std;
using namespace CEL;

int main()
{
  Vertex a(Vec3(0, 0, 0)), b(Vec3(1, 0, 0)), c(Vec3(1, 1, 0));
  vector<Vertex> vao({a, b, c});
  Triangle<int> only_t(0, 1, 2);
  vector<Triangle<int>> vbo{only_t};

  Mesh mesh(vao, vbo);

  cout << "Created a NaiveMesh" << endl;

  for (auto tri = mesh.triangles_cbegin(); tri != mesh.triangles_cend(); ++tri)
  {
    cout << tri->a->coord << endl;
    cout << tri->b->coord << endl;
    cout << tri->c->coord << endl;
  }

  return 0;
}
