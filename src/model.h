#ifndef CEL_MODEL_H
#define CEL_MODEL_H

#include <vector>
#include <memory>
#include <ostream>

#include "algebra.h"
#include "color.h"

/*
 * model.h
 * This module considers how a model is composed.
 *
 * NOTE: A Model object should be unique, but possible to have multiple
 * "instances" by relating with different transforms. (see scene.h)
 *
 * TODO: add aggregated model (and animated model)
 * TODO: Support cube and sphere
 */

namespace CEL {



// Model classes //////////////////////////////////////////////////////////////

// Base class
// supports polymorphism only through RTTI
class Model {
public:

  // Public data
  std::string name = "Model Un-named";

  // Default construct
  Model(std::string n) : name(n) {}

  // Destructor
  virtual ~Model() = default;

  // Debug info
  virtual std::string summary() const { return "Base Model."; }
  friend std::ostream& operator<<(std::ostream& os, const Model& m) {
    return os << m.summary();
  }

};


// Triangular Mesh
class Mesh : public Model {
public:

  // Simple Vertex class
  struct Vertex {
    Vec4 coord; // Vertex position in right-hand world space
    Vec3 normal; // Vertex normal
    Color color; // Vertex color

    Vertex(const Vec3& pos3, const Color& c = Color(0.5, 0.5, 0.5, 1.0))
      : coord(pos3, 1.0), normal(), color(c) {};
    Vertex(const Vec3& pos3, const Vec3& nor, const Color& c)
      : coord(pos3, 1.0), normal(nor), color(c) {};
  };

  // Triangle template class, details depend on implementation of mesh
  template<typename VertexId>
  struct TriangleImp {
    VertexId a;
    VertexId b;
    VertexId c;

    TriangleImp(VertexId a, VertexId b, VertexId c) : a(a), b(b), c(c) {};
  };

  // Simple Material class
  struct Material {
    std::string name = "CEL_Default";
    Color diffuse = {0.2, 0.2, 0.2, 1.0};
    Color ambient = {0.2, 0.2, 0.2, 1.0};
    Color specular = {0.2, 0.2, 0.2, 1.0};
    double shineness = 30;

    friend std::ostream& operator<<(std::ostream& os, Material mat) {
      return os << "name = " << mat.name
        << ", diff = " << mat.diffuse
        << ", ambi = " << mat.ambient;
    }
  };

  // Type alias
  using size_type = std::vector<Vertex>::size_type;
  using Triangle = TriangleImp<size_type>;

  // Default Constructor : empty mesh
  Mesh(std::string n = "Mesh Un-named") : Model(n) {}

  // Copy controls
  ~Mesh() override = default;
  Mesh(const Mesh& rhs) = default;
  Mesh(Mesh&& rhs) = default;

  // Useful queries
  bool empty() const { return triangles.empty(); }
  bool vertices_num() const { return vertices.size(); }
  bool triangle_num() const { return triangles.size(); }

  // Set data (by vertices index array)
  void set(std::vector<Vertex>&& va, const std::vector<size_type>& ta);

  // Set data (by vertives index triplet array)
  void set(std::vector<Vertex>&& va, std::vector<Triangle>&& ta);

  // Set material
  void set(const Material& mat) { material = mat; }
  void set(Material&& mat) { material = std::move(mat); }

  // Basic queries
  const std::vector<Vertex>& get_vertices() const { return vertices; }
  const std::vector<Triangle>& get_triangles() const { return triangles; }
  const Material& get_material() const { return material; }

  // Debug info
  std::string summary() const override;

private:

  std::vector<Vertex> vertices;
  std::vector<Triangle> triangles;
  Material material;

};


// Model Aggregates
class Aggregate : public Model {

  // Dump classs

};


} // namespace CEL

#endif
