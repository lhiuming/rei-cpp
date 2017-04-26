#ifndef CEL_SCENE_H
#define CEL_SCENE_H

#include <memory>

#include "math.h"
#include "model.h"

/*
 * scene.h
 * Define a Scene class
 *
 * TODO: support renderer queries
 * TODO: add lights
 * TODO: add audio
 */

namespace CEL {

// Instance ///////////////////////////////////////////////////////////////////

struct ModelInstance {
  ModelPtr model_p;
  Mat4 transform;
};

// Scene class ////////////////////////////////////////////////////////////////

// Base Scene
class Scene {

public:

  virtual ~Scene() {};

};


// Static Scene
class StaticScene : public Scene {

public:

  // Empty scene
  StaticScene() {}

  // Destructor: do not thing
  virtual ~StaticScene() override {}

  // Add elements
  void add_model(ModelPtr& mp, Mat4& trans);
  void add_model(ModelPtr& mp, Mat4&& trans);

private:

  std::vector<ModelInstance> models;

};

} // namespace CEL

#endif
