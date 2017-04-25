#ifndef CEL_MODEL_H
#define CEL_MODEL_H

#include <vector>

#include "math.h"

/*
 * model.h
 * This module considers how a model is composed.
 *
 * TODO: Implement 3D triangular mesh
 * TODO: Support cube and sphere
 */

namespace CEL {


// Fundamental Primitives /////////////////////////////////////////////////////

// Simple Vertex class
struct Vertex {
  Vec4 coord; // Vertex position
  Vertex(Vec3 pos3) : coord(pos3) {};
};

// Triangle template class, details depend on implementation of mesh
template<typename VertexIt>
struct Triangle {
  VertexIt a;  // Iterator to vertices
  VertexIt b;
  VertexIt c;
  Triangle(VertexIt a, VertexIt b, VertexIt c) : a(a), b(b), c(c) {};
};


// Model classes //////////////////////////////////////////////////////////////

typedef enum e_ModelType {
  MESH,
  SPHERE,
  CUBE,
  AGGREGATE  // a collection of generic models
} ModelType;

// Base
class Model {
public:
  ModelType type;

  Model() = default;
  Model(ModelType t) : type(t) {};
  virtual ~Model() = default;

  virtual const Mat4& transform() const = 0;

protected:
  Mat4 _transform;

};

typedef std::vector<Vertex>::iterator VertexIt;
typedef std::vector<Vertex>::const_iterator VertexCIt;
typedef std::vector<Triangle<VertexIt>>::iterator TriangleIt;
typedef std::vector<Triangle<VertexIt>>::const_iterator TriangleCIt;

// Trianglular Mesh
class Mesh : public Model {
public:
  // allow empty mesh
  Mesh() : Model(MESH) {};

  // allow add both vertices and triangles
  Mesh(std::vector<Vertex> va, std::vector<Triangle<int>> ta) ;

  // Destructor does nothing
  ~Mesh() override {};

  // Primitives queries
  TriangleCIt triangles_cbegin() const { return triangles.cbegin(); }
  TriangleCIt triangles_cend() const { return triangles.cend(); }

  // Get transform
  const Mat4& transform() const { return _transform; }

private:
  std::vector<Vertex> vertices;
  std::vector<Triangle<VertexIt>> triangles;

};

} // namespace CEL

#endif
