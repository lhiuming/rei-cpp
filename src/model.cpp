// source of model.h
#include "model.h"

#include <sstream>

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

// Debug print info
string Mesh::summary() const
{
  ostringstream oss;
  oss << "Mesh name: " << name
      << ", vertices : " << vertices.size()
      << ", triangles: " << triangles.size()
      << endl;
  oss << "  first 3 vertices: " << endl;
  for (int i = 0; i < 3; ++i)
  {
    auto v = vertices[i];
    oss << "    coord: " << v.coord << endl;
    oss << "    normal: " << v.normal << endl;
  }

  return oss.str();
}


} // namespace CEL
