// source of model.h
#include "model.h"

#include "console.h"

using namespace std;

namespace CEL {

// Mesh ////

// Set data by vertice id array
void Mesh::set(std::vector<Vertex>&& va, const std::vector<size_type>& ta)
{
  vertices = va;
  triangles.reserve(ta.size() / 3);
  for (size_type i = 0; i < va.size(); i += 3)
    triangles.emplace_back(ta[i], ta[i+1], ta[i+2]);
}

// Set data by triangle array
void Mesh::set(std::vector<Vertex>&& va, std::vector<Triangle>&& ta)
{
  vertices = va;
  triangles = ta;
}


} // namespace CEL
