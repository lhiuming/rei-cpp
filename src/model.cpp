// source of model.h
#include "model.h"

#include "console.h"

using namespace std;

namespace CEL {

// Mesh

// Constructor with both vertices and triangles
Mesh::Mesh(const vector<Vertex>& va, const vector<size_type>& ta)
 : Mesh(vector<Vertex>{va}, vector<size_type>{ta}) {}
Mesh::Mesh(vector<Vertex>&& vertex_array, vector<size_type>&& index_array)
 : vertices(vertex_array)
{
  // convert the index array to triangle array
  for (size_type i = 0; i < index_array.size(); i += 3)
    triangles.emplace_back(index_array[i    ],
                           index_array[i + 1],
                           index_array[i + 2]));
}

// Constructor with both vertices and triangles, alternative method
Mesh::Mesh(vector<Vertex>&& vertex_array, vector<Triangle>&& triangle_array)
: vertices(vertex_array), triangles(triangle_array) { }


} // namespace CEL
