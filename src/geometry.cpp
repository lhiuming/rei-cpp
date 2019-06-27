#include "geometry.h"

#include <vector>
#include <sstream>

using std::wstring;
using std::vector;

namespace rei {

Geometry::~Geometry() {
  // Default behaviour
}


void Mesh::set(std::vector<Vertex>&& va, const std::vector<size_type>& ta) {
  m_vertices = va;
  m_triangles.reserve(ta.size() / 3);
  for (size_type i = 0; i < va.size(); i += 3)
    m_triangles.emplace_back(ta[i], ta[i + 1], ta[i + 2]);
}


void Mesh::set(std::vector<Vertex>&& va, std::vector<Triangle>&& ta) {
  m_vertices = va;
  m_triangles = ta;
}


// Debug print info
wstring Mesh::summary() const {
  std::wostringstream oss;
  oss << "Mesh name: " << name << ", vertices : " << m_vertices.size()
      << ", triangles: " << m_triangles.size() << endl;
  oss << "  first 3 vertices: " << endl;
  for (int i = 0; i < 3; ++i) {
    auto v = m_vertices[i];
    oss << "    coord: " << v.coord << endl;
    oss << "    normal: " << v.normal << endl;
    oss << "    color:  " << v.color << endl;
  }

  return oss.str();
}


Mesh Mesh::procudure_cube(Vec3 extent, Vec3 origin, bool flip) {
  extent.x = std::abs(extent.x);
  extent.y = std::abs(extent.y);
  extent.z = std::abs(extent.z);

  vector<Vertex> vertices {24};
  vector<Triangle> triangles {12};

  int v_count = 0;
  int t_count = 0;

  // iterate on each face normal
  for (int axis = 0; axis < 3; axis++) {
    int bi_axis = (axis + 1) % 3;
    int tan_axis = (axis + 2) % 3;

    Vec3 normal = {}, bi_normal = {}, tangent = {};
    tangent[tan_axis] = 1;
    for (int side = 1; side >= -1; side -= 2) {
      normal[axis] = side;
      bi_normal[bi_axis] = side;

      // interate for front side and back side, each side with 4 vertices
      Index i_0 = v_count;
      // vertex
      for (int u = -1; u <= 1; u += 2) {
        for (int v = -1; v <= 1; v += 2) {
          Vec3 local_pos = normal + u * bi_normal + v * tangent;
          Vec3 pos = local_pos * extent + origin;
          vertices[v_count++] = {pos, normal};
        }
      }
      // triangle
      Index i_1 = i_0 + 1, i_2 = i_0 + 2, i_3 = i_0 + 3;
      if (flip) {
        triangles[t_count++] = {i_0, i_1, i_2};
        triangles[t_count++] = {i_1, i_3, i_2};
      } else {
        triangles[t_count++] = {i_0, i_2, i_1};
        triangles[t_count++] = {i_1, i_2, i_3};
      }
    }
  }

  return {L"Cube", std::move(vertices), std::move(triangles)};
}

} // namespace rei