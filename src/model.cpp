// source of model.h
#include "model.h"

#include "console.h"

using namespace std;

namespace CEL {

// Mesh

// Constructor with both vertices and triangles
Mesh::Mesh(const vector<Vertex>& va, const vector<size_type>& ta)
 : Mesh(vector<Vertex>(va), vector<size_type>(ta)) {};
Mesh::Mesh(vector<Vertex>&& va, vector<size_type>&& ta)
 : vertices(va)
{
  // TODO check the size of va ta
  // TODO check the maximum value in ta

  // convert the vertices id to iterator
  VertexIt offset = vertices.begin();
  for (size_type i = 0; i < ta.size(); i += 3) {
    triangles.push_back(Triangle(offset + ta[i    ],
                                 offset + ta[i + 1],
                                 offset + ta[i + 2]));
    console << "Put triangles from vertex " <<triangles[i/3].a - offset 
      << endl;
  }
}

// Constructor with both vertices and triangles, alternative method
Mesh::Mesh(vector<Vertex>&& va, vector<IndexTriangle>&& ta) : vertices(va)
{
  VertexIt offset = vertices.begin();
  for (const auto& ti : ta)
  {
    // Convert from index to interator
    Triangle t(offset + ti.a, offset + ti.b, offset + ti.c);
    triangles.push_back(t);
  }
}


} // namespace CEL
