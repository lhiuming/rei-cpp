#ifndef CEL_MODEL_H
#define CEL_MODEL_H

#include <vector>
#include <memory>

#include "algebra.h"

/*
 * model.h
 * This module considers how a model is composed.
 *
 * NOTE: A Model object shoule be unique, but possible to have multiple
 * "instances" by relating with different transforms. (see scene.h)
 *
 * TODO: add aggregated model (and animated model)
 * TODO: support move constructors
 * TODO: Support cube and sphere
 */

namespace CEL {

typedef enum e_ModelType {
  MESH,
  SPHERE,
  CUBE,
  AGGREGATE  // a collection of generic models
} ModelType;

// Simple Vertex class
struct Vertex {
  Vec4 coord; // Vertex position
  Vertex(Vec3 pos3) : coord(pos3, 1.0) {};
};

// Triangle template class, details depend on implementation of mesh
template<typename VertexIt>
struct Triangle {
  VertexIt a;  // Iterator to vertices
  VertexIt b;
  VertexIt c;
  Triangle(VertexIt a, VertexIt b, VertexIt c) : a(a), b(b), c(c) {};
};

typedef std::vector<Vertex>::iterator VertexIt;
typedef std::vector<Vertex>::const_iterator VertexCIt;
typedef Triangle<VertexIt> TriangleType;
typedef std::vector<TriangleType>::iterator TriangleIt;
typedef std::vector<TriangleType>::const_iterator TriangleCIt;


// Model classes //////////////////////////////////////////////////////////////

// Base class
// supports polymophism only through RTTI
class Model {
public:

  virtual ~Model() = default;

};


// Trianglular Mesh
class Mesh : public Model {

public:

  // Dont allow empty mesh
  Mesh() = delete;

  // Constructor with both vertices and triangles
  Mesh(std::vector<Vertex> va, std::vector<Triangle<int>> ta);

  // Destructor; we have only standard containers
  ~Mesh() override = default;

  // Primitives queries
  TriangleCIt triangles_cbegin() const { return triangles.cbegin(); }
  TriangleCIt triangles_cend() const { return triangles.cend(); }

private:
  std::vector<Vertex> vertices;
  std::vector<TriangleType> triangles;

};


} // namespace CEL

#endif
