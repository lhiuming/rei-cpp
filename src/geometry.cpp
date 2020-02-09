#include "geometry.h"

#include <sstream>
#include <unordered_map>
#include <vector>

#include "debug.h"
#include "rmath.h"

using std::vector;
using std::wstring;

namespace rei {

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
  oss << "vertices : " << m_vertices.size()
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

  return {std::move(vertices), std::move(triangles)};
}

Mesh Mesh::procudure_sphere_icosahedron(int subdivision, double radius, Vec3 origin, bool flip) {
  /*
   * Use snub tetrahedron.
   * ref: https://en.wikipedia.org/wiki/Icosahedron
   */

  static const real_t sqrt_5 = std::sqrt(5);
  static const real_t g = (sqrt_5 + 1) / 2; // golden ratio

  constexpr int regular_triangle_num = 20;
  constexpr int regular_vertex_num = 12;

  // Hard-coded vertex coordinates
  // NOTE: the order is designed to fit triangle-strip construction
  static const Vec3 poses[12] = {
    {1, 0, -g}, // 0, start of first strip
    {g, -1, 0},
    {g, 1, 0},
    {1, 0, g},
    {0, g, 1},
    {-1, 0, g},
    {0, g, -1}, // 6, for second strip
    {-g, 1, 0},
    {-1, 0, -g}, // 8, for thrid strip
    {-g, -1, 0},
    {0, -g, 1}, // 10, for fourth strip
    {0, -g, -1},
  };

  // Hard-coded triangle indicies
  // NOTE: totally 5 strip, each strip contains 4 triangle, accessed with order {0, 2, 1} etc. (see
  // pattern below)
  constexpr int regular_strip_num = 5;
  constexpr int strip_vertex_num = 6;
  static const int strips[5][6] = {
    {0, 1, 2, 3, 4, 5},
    {0, 2, 6, 4, 7, 5},
    {0, 6, 8, 7, 9, 5},
    {0, 8, 11, 9, 10, 5},
    {0, 11, 1, 10, 3, 5},
  };

  subdivision = (std::max)(subdivision, 0);
  if (subdivision > 8) {
    subdivision = 8;
    REI_WARNING("icosahadron subdivision exceeding 8 is not supported");
  }

  const Size vertex_num
    = regular_vertex_num
      + Size(30 * (pow_i(4, subdivision) - 1) / 3); // deduction is trivial/ignored
  const Size triangle_num = regular_triangle_num * Size(pow_i(4, subdivision));
  vector<Vertex> vertices {vertex_num};
  vector<Triangle> triangles {triangle_num};

  // Important helper data.
  // Representing each vertex uniquely by rational barycentric coordinates, on the subdivided
  // icosahedron surface NOTE supporting upto 16-times subdivision
  struct BarycentricID {
    uint8_t toplevel_index[4];
    uint16_t v1;          // coordinate on axis index0->index1
    uint16_t v2;          // coordinate on axis index0->index2
    uint16_t subdiv_size; // number of segmentation on the edge

    void step01() {
      REI_ASSERT(v1 < subdiv_size);
      v1++;
    }

    void step02() {
      REI_ASSERT(v2 < subdiv_size);
      v2++;
    }

    void step12() {
      REI_ASSERT(v1 > 0);
      REI_ASSERT(v2 < subdiv_size);
      v1--;
      v2++;
    }

    void step21() {
      REI_ASSERT(v2 > 0);
      REI_ASSERT(v1 < subdiv_size);
      v1++;
      v2--;
    }

    // convert from coordinate to interpolated vertex position on the unit sphere
    Vec3 get_pos() const {
      Vec3 pos;
      if (v1 + v2 <= subdiv_size) {
        Vec3 base = poses[toplevel_index[0]];
        pos = base;
        if (v1 > 0) {
          Vec3 dif = poses[toplevel_index[1]] - base;
          pos += (v1 / double(subdiv_size)) * dif;
        }
        if (v2 > 0) {
          Vec3 dif = poses[toplevel_index[2]] - base;
          pos += (v2 / double(subdiv_size)) * dif;
        }
      } else {
        Vec3 base = poses[toplevel_index[3]];
        pos = base;
        if (v1 < subdiv_size) {
          Vec3 dif = poses[toplevel_index[2]] - base;
          pos += ((subdiv_size - v1) / double(subdiv_size)) * dif;
        }
        if (v2 < subdiv_size) {
          Vec3 dif = poses[toplevel_index[1]] - base;
          pos += ((subdiv_size - v2) / double(subdiv_size)) * dif;
        }
      }
      return pos.normalized();
    }

    // a unique hash value for each vertex; never collide.
    std::size_t hash() const {
      REI_ASSERT(toplevel_index[0] < 12);
      REI_ASSERT(toplevel_index[1] < 12);
      REI_ASSERT(toplevel_index[2] < 12);
      REI_ASSERT(toplevel_index[3] < 12);
      // if is a top-level vertex
      if (v1 == 0 && v2 == 0) { return toplevel_index[0]; }
      if (v1 == subdiv_size && v2 == 0) { return toplevel_index[1]; }
      if (v1 == 0 && v2 == subdiv_size) { return toplevel_index[2]; }
      if (v1 == subdiv_size && v2 == subdiv_size) { return toplevel_index[3]; }
      // if is on a top-level edge
      if (v2 == 0) {
        return (toplevel_index[0]) ^ (size_t(toplevel_index[1]) << 8) ^ (size_t(v1) << 16);
      }
      if (v1 == 0) {
        return (toplevel_index[0]) ^ (size_t(toplevel_index[2]) << 8) ^ (size_t(v2) << 16);
      }
      if (v2 == subdiv_size) {
        return (toplevel_index[2]) ^ (size_t(toplevel_index[3]) << 8) ^ (size_t(v1) << 16);
      }
      if (v1 == subdiv_size) {
        return (toplevel_index[1]) ^ (size_t(toplevel_index[3]) << 8) ^ (size_t(v2) << 16);
      }
      // if is on a subdived face
      size_t ret;
      if (v1 + v2 < subdiv_size) {
        ret = size_t(toplevel_index[0]);
        ret = ret ^ (size_t(toplevel_index[1]) << 8) ^ (size_t(v1) << 24);
        ret = ret ^ (size_t(toplevel_index[2]) << 32) ^ (size_t(v2) << 48);
      } else {
        ret = size_t(toplevel_index[2]);
        ret = ret ^ (size_t(toplevel_index[3]) << 8) ^ (size_t(v1) << 24);
        ret = ret ^ (size_t(toplevel_index[1]) << 32) ^ (size_t(v2) << 48);
      }
      return ret;
    }

    bool operator==(const BarycentricID& other) const { return hash() == other.hash(); }
  };

  // Map the allocated index by the coordination, to identify the edge that are shared among strips
  Size next_index = 0;
  std::unordered_map<std::size_t, Size> indices_allc {triangle_num * 3};
  auto get_indices = [&](const BarycentricID& id, bool& is_new) -> Size {
    auto pos = indices_allc.find(id.hash());
    Size ret;
    if (pos == indices_allc.end()) {
      ret = next_index++;
      indices_allc.insert({id.hash(), ret});
      is_new = true;
    } else {
      ret = pos->second;
      is_new = false;
    }
    return ret;
  };

  const int subdiv_size = pow_i(2, subdivision);

  // NOTE: allocate index creating the vertex entry if not already
  auto find_and_create = [&](const BarycentricID& id) -> Size {
    bool is_new = false;
    Size index = get_indices(id, is_new);
    if (is_new) {
      // REMAK: vertex creation
      Vec3 local_pos = id.get_pos();
      vertices[index] = {local_pos * radius + origin, local_pos.normalized()};
    }
    return index;
  };

  size_t triangle_count = 0; // counting for insert
  auto fill_ministrip = [&](BarycentricID tri[3]) {
    // ring buffer for alteranting index
    Size buffer[3] = {};
    for (int i = 0; i < 3; i++) { // initialize
      buffer[i] = find_and_create(tri[i]);
    }
    // interate through all triangle in this mini strip
    const int triangle_num = subdiv_size * 2; // number of triangle in this mini strip
    for (int i = 0; i < triangle_num; i++) {
      // TODO handle flipping here
      if (i % 2 == 0) { // even
        triangles[triangle_count++] = {buffer[i % 3], buffer[(i + 1) % 3], buffer[(i + 2) % 3]};
      } else { // odd
        triangles[triangle_count++] = {buffer[i % 3], buffer[(i + 2) % 3], buffer[(i + 1) % 3]};
      }
      // move along the strip (going from index0 to index2)
      if (i != triangle_num - 1) {
        if (i % 2 == 0) { // even
          tri[i % 3].step01();
          tri[i % 3].step02();
        } else { // odd
          tri[i % 3].step12();
          tri[i % 3].step02();
        }
        Size index = find_and_create(tri[i % 3]);
        buffer[i % 3] = index;
      }
    }
  };

  // Generating mini-strips in a quadrilateral-shape
  auto fill_quadrilateral = [&](int i[4]) {
    BarycentricID b0
      = {{uint16_t(i[0]), uint16_t(i[1]), uint16_t(i[2]), uint16_t(i[3])}, 0, 0, subdiv_size};
    BarycentricID b1
      = {{uint16_t(i[0]), uint16_t(i[1]), uint16_t(i[2]), uint16_t(i[3])}, 1, 0, subdiv_size};
    BarycentricID b2
      = {{uint16_t(i[0]), uint16_t(i[1]), uint16_t(i[2]), uint16_t(i[3])}, 0, 1, subdiv_size};
    for (int i = 0; i < subdiv_size; i++) {
      BarycentricID init_tri[3] = {b0, b1, b2};
      // fill the strip going along index0->index2 axis
      fill_ministrip(init_tri);
      // shift the strip along index0->index1 axis
      if (i != subdiv_size - 1) {
        b0.step01();
        b1.step01();
        b2.step01();
      }
    }
  };

  // Generate the mesh using all the buffers, allocators and routine defined above
  for (int i = 0; i < regular_strip_num; i++) {
    // Break-up the top-level strip into two quadrilateral-shaped mini-strips, for subdivision
    int ministrip[] = {(strips[i][0]), (strips[i][2]), (strips[i][1]), (strips[i][3])};
    int ministrip2[] = {(strips[i][2]), (strips[i][4]), (strips[i][3]), (strips[i][5])};
    fill_quadrilateral(ministrip);
    fill_quadrilateral(ministrip2);
  }

  return {std::move(vertices), std::move(triangles)};
}

Geometries::Geometries() : m_next_id(1) {}

GeometryPtr Geometries::create(Name&& name, Mesh&& mesh) {
  REI_ASSERT(m_next_id != Geometry::kInvalidID);
  auto ptr = std::shared_ptr<Geometry>(new Geometry(m_next_id, std::move(name), std::move(mesh)));
  m_geometries.insert(ptr->id(), ptr);
  m_next_id += 1;
  return ptr;
}

void Geometries::destroy(GeometryPtr& ptr) {
  Geometry::ID k = ptr->id();
  REI_ASSERT(k != Geometry::kInvalidID);
  int count = m_geometries.erase(k);
  REI_ASSERT(count == 1);
}

} // namespace rei