// source of scene.h
#include "scene.h"

#include <iostream>
#include <vector>
#include <memory>

#include "algebra.h"
#include "model.h"

using namespace std;

namespace CEL {

// Static scene ///////////////////////////////////////////////////////////////
////

// Add a model
void StaticScene::add_model(const ModelPtr& mp, const Mat4& trans)
{
  models.push_back(ModelInstance{mp, trans});
}

// Get the reference to the model container
const Scene::ModelContainer& StaticScene::get_models() const
{
    return models;
} // end get_models


} // namespace CEL
