#ifndef CEL_MODEL_H
#define CEL_MODEL_H

#include <vector>
#include <memory>

#include "math.h"

/*
 * model.h
 * This module considers how a model is composed.
 *
 * NOTE: A Model object shoule be unique, but possible to have multiple
 * "instances" by relating with different transforms. (see Aggreate, also
 * see scene.h)
 *
 * TODO: add aggregated model
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

typedef std::vector<Vertex>::iterator VertexIt;
typedef std::vector<Vertex>::const_iterator VertexCIt;
typedef Triangle<VertexIt> TriangleType;
typedef std::vector<TriangleType>::iterator TriangleIt;
typedef std::vector<TriangleType>::const_iterator TriangleCIt;


// Model classes //////////////////////////////////////////////////////////////

// Base class
class Model {
public:
  const ModelType type;

  virtual ~Model() = default;

protected:

  // Initialized the ModelType
  Model(ModelType t) : type(t) {};

};


// Trianglular Mesh
class Mesh : public Model {

public:

  // Dont allow empty mesh
  Mesh() = delete;

  // Constructor with both vertices and triangles
  // TODO: support move
  Mesh(std::vector<Vertex> va, std::vector<Triangle<int>> ta);

  // Destructor is default, since we have only standard containers
  ~Mesh() override = default;

  // Primitives queries
  TriangleCIt triangles_cbegin() const { return triangles.cbegin(); }
  TriangleCIt triangles_cend() const { return triangles.cend(); }

private:
  std::vector<Vertex> vertices;
  std::vector<TriangleType> triangles;

};


typedef std::shared_ptr<Model> ModelPtr;
typedef std::vector<ModelPtr>::iterator ModelPtrIt;
typedef std::vector<ModelPtr>::const_iterator ModelPtrCIt;


// Model AGGREGATE
class Aggregate : public Model {

public:

  typedef std::vector<Mat4>::size_type size_type;

  // Dont alloow emtpy aggregated
  Aggregate() = delete;

  // Simple constructors
  // TODO: move
  Aggregate(std::vector<ModelPtr> models); // with default transform
  Aggregate(std::vector<ModelPtr> models, std::vector<Mat4> trans)
    : Model(AGGREGATE), models(models), transforms(trans) {};

  // Destructor: STL takes care of the memory
  ~Aggregate() override {};

  // Models queries
  size_type size() const { return transforms.size(); }
  const std::vector<ModelPtr>& get_models() const { return models; }
  const std::vector<Mat4>& get_transforms() const { return transforms; }

private:
  std::vector<ModelPtr> models;
  std::vector<Mat4> transforms;

};


} // namespace CEL

#endif
