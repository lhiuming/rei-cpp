// source of model.h
#include "model.h"

using namespace std;

namespace CEL {

// Mesh

// Constructor with both vertices and triangles
// TODO: exception when vertex id is invalid?
Mesh::Mesh(vector<Vertex> va, vector<Triangle<int>> ta)
 : vertices(va)
{
  VertexIt offset = vertices.begin();
  for (auto tri : ta)
    triangles.push_back(Triangle<VertexIt>(offset + tri.a,
                                           offset + tri.b,
                                           offset + tri.c ));
}


} // namespace CEL
