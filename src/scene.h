#ifndef CEL_SCENE_H
#define CEL_SCENE_H

#include <memory>

#include "algebra.h"
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

using ModelPtr = std::shared_ptr<Model>;

// Model
// NOTE: you can add more features to help rendering !
struct ModelInstance {
  ModelPtr pmodel;
  Mat4 transform;

  // More
};

// Scene class ////////////////////////////////////////////////////////////////
////

// Base Scene
class Scene {
public:
  // Name for convinient
  std::string name;

  // Member types
  typedef std::vector<ModelInstance> ModelContainer;

  // Default constructor
  Scene(std::string n = "Un-named Scene") : name(n) {}

  // Destructor
  virtual ~Scene() {};

  // Get elements
  virtual const ModelContainer& get_models() const = 0;

  // Debug info
  virtual std::string summary() const { return "Base Scene"; }

  friend std::ostream& operator<<(std::ostream& os, const Scene& s) { return os << s.summary(); }
};

// Static Scene
class StaticScene : public Scene {
public:
  // Default constructor; an empty scene
  StaticScene(std::string n = "Un-named StaticScene") : Scene(n) {}

  // Destructor
  ~StaticScene() override = default;

  // Add elements
  void add_model(ModelInstance&& mi) { models.emplace_back(std::move(mi)); }
  void add_model(const ModelPtr& mp, const Mat4& trans) { models.push_back({mp, trans}); }

  // Get models
  const ModelContainer& get_models() const override { return models; }

  // Debug info
  std::string summary() const override;

private:
  std::vector<ModelInstance> models;
};

} // namespace CEL

#endif
