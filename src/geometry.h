#ifndef REI_GEOMETRY_H
#define REI_GEOMETRY_H

#include <vector>
#include <string>

#include "common.h"
#include "algebra.h"
#include "color.h"
#include "graphic_handle.h"

namespace rei {

// Base class for all geometry object
class Geometry : public NoCopy {
public:
  Geometry(std::wstring name) : name(name) {};
  virtual ~Geometry() = 0;

  Geometry(Geometry&& other) = default;

  virtual GeometryHandle get_graphic_handle() const = 0;
  virtual void set_graphic_handle(const GeometryHandle& h) = 0 ;

  // Debug info
  virtual std::wstring summary() const { return L"<Base Geomtry>"; }
  friend std::wostream& operator<<(std::wostream& os, const Geometry& g) { return os << g.summary(); }

protected:
  std::wstring name;
};

typedef std::shared_ptr<Geometry> GeometryPtr;

// Triangular Mesh
class Mesh : public Geometry {
public:
  // Simple Vertex class
  struct Vertex {
    Vec4 coord;  // Vertex position in right-hand world space
    Vec3 normal; // Vertex normal
    Color color; // Vertex color

    Vertex() {}
    Vertex(const Vec3& pos3) : coord(pos3, 1.0), normal(pos3.normalized()), color(Colors::white) {}
    Vertex(const Vec3& pos3, const Vec3& nor, const Color& c = Colors::white)
        : coord(pos3, 1.0), normal(nor), color(c) {}
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

  [[deprecated]] Mesh(std::string n) : Geometry(L"deprecated") { REI_DEPRECATED }
  Mesh(std::wstring n = L"Mesh Un-named") : Geometry(n) {}
  Mesh(std::wstring name, std::vector<Vertex> vertices, std::vector<Triangle>&& triangles) : Geometry(name), m_vertices(vertices), m_triangles(triangles) {}

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

  GeometryHandle get_graphic_handle() const override { return rendering_handle; }
  void set_graphic_handle(const GeometryHandle& h) override { rendering_handle = h; }

  // Debug info
  std::wstring summary() const override;

  // Some mesh-related utilities
  // Set `flip` to true if the orientation handness is different from coordinate handness
  static Mesh procudure_cube(Vec3 extent = {1, 1, 1}, Vec3 origin = {0, 0, 0}, bool flip = false);
  static Mesh procudure_sphere(double radius = 1, Vec3 origin = {0, 0, 0});

private:
  std::vector<Vertex> m_vertices;
  std::vector<Triangle> m_triangles;

  GeometryHandle rendering_handle;
};

typedef std::shared_ptr<Mesh> MeshPtr;

}

#endif // !REI_GEOMETRY_H
