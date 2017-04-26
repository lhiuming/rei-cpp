// source of model.h
#include "model.h"

using namespace std;

namespace CEL {

// Mesh

// Constructor with both vertices and triangles
Mesh::Mesh(vector<Vertex> va, vector<Triangle<int>> ta)
 : Model(MESH), vertices(va)
{
  VertexIt offset = vertices.begin();
  for (auto tri : ta)
    triangles.push_back(Triangle<VertexIt>(offset + tri.a,
                                           offset + tri.b,
                                           offset + tri.c ));
}

// Aggregate

Aggregate::Aggregate(std::vector<ModelPtr> models)
 : Model(AGGREGATE), models(models)
{
  for(size_t i = 0; i < models.size(); ++i) {
    transforms.push_back(Mat4());
  }
};



} // namespace CEL
