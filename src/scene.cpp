// source of scene.h
#include "scene.h"

#include <iostream>
#include <vector>
#include <memory>

#include "algebra.h"
#include "model.h"

using namespace std;

namespace CEL {

// a default model (a big plane) //
Vertex a(Vec3( 10, -5, -30));
Vertex b(Vec3(-10, -5, -30));
Vertex c(Vec3(-10, -5, -50));
Vertex d(Vec3( 10, -5, -50));
vector<Vertex> va{a, b, c, d};
ModelInstance plane_inst{
  ModelPtr(new Mesh(std::move(va), {2, 1, 0, 3, 2, 0})),
  Mat4::I() };
vector<ModelInstance> default_models{plane_inst};

// Static scene ///////////////////////////////////////////////////////////////
////

// Add a model
void StaticScene::add_model(const ModelPtr& mp, const Mat4& trans)
{
  models.push_back(ModelInstance{mp, trans});
  no_models = false;
}

// Get the reference to the model container
const Scene::ModelContainer& StaticScene::get_models() const
{
  if (no_models) { // return a default model (a big plane)
    return default_models;
  } else {
    return models;
  }
} // end get_models


} // namespace CEL
