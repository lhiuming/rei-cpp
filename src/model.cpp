// source of model.h
#include "model.h"

using namespace std;

namespace CEL {

// Constructor: allow add both vertices and triangles
NaiveMesh::NaiveMesh(vector<Vertex> va, vector<Triangle<int>> ta)
: vertices(va)
{
  VertexIt offset = vertices.begin();
  for (auto tri : ta)
    triangles.push_back(Triangle<VertexIt>(offset + tri.a,
                                           offset + tri.b,
                                           offset + tri.c ));
}

} // namespace CEL
