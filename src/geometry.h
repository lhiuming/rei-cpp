#ifndef REI_GEOMETRY_H
#define REI_GEOMETRY_H

#include <vector>
#include <string>

#include "common.h"
#include "algebra.h"
#include "color.h"

namespace rei {

// Base class for all geometry object
class Geometry : public NoCopy {
public:
  Geometry(std::wstring name) : name(name) {};
  virtual ~Geometry() = 0;

  // Debug info
  virtual std::wstring summary() const { return L"<Base Geomtry>"; }
  friend std::wostream& operator<<(std::wostream& os, const Geometry& g) { return os << g.summary(); }

protected:
  std::wstring name;
};

// Triangular Mesh
class Mesh : public Geometry {
public:
  // Simple Vertex class
  struct Vertex {
    Vec4 coord;  // Vertex position in right-hand world space
    Vec3 normal; // Vertex normal
    Color color; // Vertex color

    Vertex(const Vec3& pos3, const Color& c = Color(0.5f, 0.5f, 0.5f, 1.0f))
        : coord(pos3, 1.0), normal(), color(c) {};
    Vertex(const Vec3& pos3, const Vec3& nor, const Color& c)
        : coord(pos3, 1.0), normal(nor), color(c) {};
  };

  // Triangle template class, details depend on implementation of mesh
  template <typename VertexId>
  struct TriangleImp {
    VertexId a;
    VertexId b;
    VertexId c;

    TriangleImp(VertexId a, VertexId b, VertexId c) : a(a), b(b), c(c) {};
  };

  // Type alias
  using size_type = std::vector<Vertex>::size_type;
  using Triangle = TriangleImp<size_type>;

  [[deprecated]] Mesh(std::string n) : Geometry(L"deprecated") { DEPRECATED }
  Mesh(std::wstring n = L"Mesh Un-named") : Geometry(n) {}

  // Set data (by vertices index array)
  void set(std::vector<Vertex>&& va, const std::vector<size_type>& ta);
  // Set data (by vertives index triplet array)
  void set(std::vector<Vertex>&& va, std::vector<Triangle>&& ta);
  // Basic queries
  const std::vector<Vertex>& get_vertices() const { return vertices; }
  const std::vector<Triangle>& get_triangles() const { return triangles; }

 // Convinient queries
  bool empty() const { return triangles.empty(); }
  bool vertices_num() const { return vertices.size(); }
  bool triangle_num() const { return triangles.size(); }

  // Debug info
  std::wstring summary() const override;

private:
  std::vector<Vertex> vertices;
  std::vector<Triangle> triangles;
};

}

#endif // !REI_GEOMETRY_H
