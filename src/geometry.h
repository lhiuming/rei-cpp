#ifndef REI_GEOMETRY_H
#define REI_GEOMETRY_H

#include <string>
#include <vector>

#include "algebra.h"
#include "color.h"
#include "common.h"
#include "container_utils.h"
#include "renderer/graphics_handle.h"

namespace rei {

// fwd decl
class MeshData;
class Geometries;

// Triangular Mesh Data
class Mesh {
public:
  // Simple Vertex class
  struct Vertex {
    Vec3 coord;  // Vertex position in right-hand world space
    Vec3 normal; // Vertex normal
    Color color; // Vertex color

    Vertex() {}
    Vertex(const Vec3& pos3) : coord(pos3), normal(pos3.normalized()), color(Colors::white) {}
    Vertex(const Vec3& pos3, const Vec3& nor, const Color& c = Colors::white)
        : coord(pos3), normal(nor), color(c) {}
  };

  // Triangle template class, details depend on implementation of mesh
  template <typename VertexId>
  struct TriangleTpl {
    VertexId a;
    VertexId b;
    VertexId c;

    TriangleTpl() {}
    TriangleTpl(VertexId a, VertexId b, VertexId c) : a(a), b(b), c(c) {};
  };

  // Type alias
  using size_type = std::vector<Vertex>::size_type;
  using Size = std::vector<Vertex>::size_type;
  using Triangle = TriangleTpl<size_type>;
  using Index = size_type;

  Mesh() = default;
  Mesh(std::vector<Vertex> vertices, std::vector<Triangle>&& triangles)
      : m_vertices(vertices), m_triangles(triangles) {}

  Mesh(Mesh&& other) = default;

  // Set data (by vertices index array)
  void set(std::vector<Vertex>&& va, const std::vector<size_type>& ta);
  // Set data (by vertives index triplet array)
  void set(std::vector<Vertex>&& va, std::vector<Triangle>&& ta);

  // Basic queries
  const std::vector<Vertex>& get_vertices() const { return m_vertices; }
  const std::vector<Triangle>& get_triangles() const { return m_triangles; }

  // Convinient queries
  bool empty() const { return m_triangles.empty(); }
  bool vertices_num() const { return m_vertices.size(); }
  bool triangle_num() const { return m_triangles.size(); }

  // Debug info
  std::wstring summary() const;

  // Some mesh-related utilities
  // Set `flip` to true if the orientation handness is different from coordinate handness
  static Mesh procudure_cube(Vec3 extent = {1, 1, 1}, Vec3 origin = {0, 0, 0}, bool flip = false);
  static Mesh procudure_sphere(
    int quality = 2, double radius = 1, Vec3 origin = {0, 0, 0}, bool flip = false) {
    return procudure_sphere_icosahedron(quality, radius, origin, flip);
  }
  static Mesh procudure_sphere_icosahedron(
    int subdivision = 0, double radius = 1, Vec3 origin = {0, 0, 0}, bool flip = false);

private:
  std::vector<Vertex> m_vertices;
  std::vector<Triangle> m_triangles;
};

// Base class for all geometry object
class Geometry : public NoCopy {
public:
  using ID = std::uintptr_t;
  static constexpr ID kInvalidID = 0;

  ID id() const { return m_id; }
  const Name name() const { return m_name; }

  const Mesh* mesh() const { return &m_mesh; }

  // Debug info
  virtual std::wstring summary() const { return L"<Base Geomtry>"; }
  friend std::wostream& operator<<(std::wostream& os, const Geometry& g) {
    return os << g.summary();
  }

private:
  ID m_id;
  Name m_name;
  // TODO make variant
  Mesh m_mesh;

  friend Geometries;

  Geometry(ID id, Name name) : m_id(id), m_name(name) {};
  Geometry(ID id, Name name, Mesh&& mesh) : m_id(id), m_name(name), m_mesh(std::move(mesh)) {};
};

typedef std::shared_ptr<Geometry> GeometryPtr;

class Geometries {
public:
  Geometries();
  ~Geometries() = default;

  GeometryPtr create(Name&& name, Mesh&& mesh);
  void destroy(GeometryPtr& ptr);

private:
  Geometry::ID m_next_id;
  Hashmap<Geometry::ID, GeometryPtr> m_geometries;
};

} // namespace rei

#endif // !REI_GEOMETRY_H
