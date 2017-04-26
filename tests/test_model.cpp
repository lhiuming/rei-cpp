// Test model module

#include <iostream>
#include <model.h>

using namespace std;
using namespace CEL;

int main()
{
  // set vertices
  Vertex a(Vec3(0, 0, 0)), b(Vec3(1, 0, 0)), c(Vec3(1, 1, 0));
  vector<Vertex> vao({a, b, c});

  // set triangle
  Triangle<int> only_t(0, 1, 2);
  vector<Triangle<int>> vbo{only_t};

  // make the mesh
  Mesh mesh(vao, vbo);

  cout << "Created a Mesh" << endl;

  for (auto tri = mesh.triangles_cbegin(); tri != mesh.triangles_cend(); ++tri)
  {
    cout << tri->a->coord << endl;
    cout << tri->b->coord << endl;
    cout << tri->c->coord << endl;
  }

  // copy a new mesh mode
  ModelPtr meshp = make_shared<Mesh>(mesh);
  Aggregate meshagg = Aggregate({meshp, meshp});

  cout << "Created a Aggreate with 2 instances of the same mesh" << endl;

  for (const ModelPtr mp : meshagg.get_models()) {
    for (auto tri = dynamic_cast<Mesh*>(mp.get())->triangles_cbegin();
              tri != dynamic_cast<Mesh*>(mp.get())->triangles_cend();
              ++tri)
    {
      cout << "mesh 1" << endl;
      cout << tri->a->coord << endl;
      cout << tri->b->coord << endl;
      cout << tri->c->coord << endl;
     }
  }


  return 0;
}
