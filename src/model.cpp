// source of model.h
#include "model.h"

using namespace std;

namespace CEL {

// Mesh

// Constructor with both vertices and triangles
Mesh::Mesh(vector<Vertex>&& va, const vector<size_type>& ta)
 : vertices(va)
{
  // TODO check the size of va ta
  // TODO check the maximum value in ta

  // convert the vertices id to iterator
  VertexIt offset = vertices.begin();
  for (size_type i = 0; i < ta.size(); i += 3)
    triangles.push_back(Triangle(offset + ta[i    ],
                                 offset + ta[i + 1],
                                 offset + ta[i + 2]));
}


} // namespace CEL
