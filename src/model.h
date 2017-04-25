#ifndef CEL_MODEL_H
#define CEL_MODEL_H

#include <vector>

#include "math.h"

/*
 * model.h
 * This module considers how a model is composed.
 *
 * TODO: Implement 3D triangular mesh
 */

namespace CEL {

//

// Fundamental Primitives /////////////////////////////////////////////////////

// Simple Vertex class
struct Vertex {
  Vec3 pos; // Vertex position

  Vertex(Vec3 pos) : pos(pos) {};

};

// Triangle template class, details depend on implementation of mesh
template<typename VertexIt>
struct Triangle {
  VertexIt a;  // Iterator to vertices
  VertexIt b;
  VertexIt c;

  Triangle(VertexIt a, VertexIt b, VertexIt c) : a(a), b(b), c(c) {};
};

// Mesh Interface /////////////////////////////////////////////////////////////
class Mesh {
public:
  virtual ~Mesh() {};

};

// Naive Mesh /////////////////////////////////////////////////////////////////
class NaiveMesh : public Mesh {

public:

  typedef std::vector<Vertex>::const_iterator VertexCIt;
  typedef std::vector<Triangle<VertexCIt>>::const_iterator TriangleCIt;

  // Constructor
  NaiveMesh() = delete;
  NaiveMesh(std::vector<Vertex> &vertices,
    std::vector<Triangle<VertexCIt>> &triangles) :
    vertices(vertices), triangles(triangles) {};

  // Destructor does nothing
  ~NaiveMesh() {};

  // Primitives queries
  TriangleCIt triangles_cbegin() const { return triangles.cbegin(); }
  TriangleCIt triangles_cend() { return triangles.cend(); }

private:

  const std::vector<Vertex> vertices;
  const std::vector<Triangle<VertexCIt>> triangles;

};






// Primitives for the Halfedge mesh ///////////////////////////////////////////


} // namespace CEL

#endif
