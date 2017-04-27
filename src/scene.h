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
////

// Model
// NOTE: you can add more features to help rendering !
struct ModelInstance {
  ModelPtr model_p;
  Mat4 transform;

  // More
};

// Scene class ////////////////////////////////////////////////////////////////
////

// Base Scene
class Scene {
public:

  // Destructor
  virtual ~Scene() {};

  // Get elements
  typedef std::vector<ModelInstance> ModelContainer;
  virtual const ModelContainer& get_models() const = 0;

};


// Static Scene
class StaticScene : public Scene {

public:

  // Default constructor; an empty scene
  StaticScene() {}

  // Destructor
  virtual ~StaticScene() override {}

  // Add elements
  void add_model(ModelPtr& mp, Mat4& trans);
  void add_model(ModelPtr& mp, Mat4&& trans);

  // Get models
  typedef std::vector<ModelInstance> ModelContainer;
  const ModelContainer& get_models() const override { return models; }

private:

  std::vector<ModelInstance> models;

};

} // namespace CEL

#endif
