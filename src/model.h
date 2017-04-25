#ifndef CEL_MODEL_H
#define CEL_MODEL_H

#include <vector>

#include "math"

/*
 * model.h
 * This module considers how a model is composed.
 *
 * TODO: Implement 3D triangular mesh
 */

namespace CEL {

// Some primitives for the Halfedge mesh
struct Vertex {
  Vec3 pos; // position

};
struct Edge {

};
struct Halfedge {

};
struct Face {

};

// Triangular Mesh
class Mesh {

public:

  Mesh();
  ~Mesh();

private:

  vector<Vertex> vertices;
  vector<Edge> edges;
  vector<Halfedge> halfedges;
  vector<Face> faces; 

};

} // namespace CEL

#endif
