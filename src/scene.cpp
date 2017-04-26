// source of scene.h
#include "scene.h"

#include <iostream>


namespace CEL {

// Static scene

void StaticScene::add_model(ModelPtr& mp, Mat4& trans)
{
  models.push_back(ModelInstance{mp, trans});
}

void StaticScene::add_model(ModelPtr& mp, Mat4&& trans)
{
  models.push_back(ModelInstance{mp, trans});
}

} // namespace CEL
