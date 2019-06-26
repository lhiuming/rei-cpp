#ifndef REI_SCENE_H
#define REI_SCENE_H

#include <memory>

#include "algebra.h"
#include "model.h"
#include "graphic_handle.h"

/*
 * scene.h
 * Define a Scene class
 *
 * TODO: support renderer queries
 * TODO: add lights
 * TODO: add audio
 */

namespace rei {

// Scene class ////////////////////////////////////////////////////////////////
////

// Base Scene
class Scene {
public:
  // Member types
  typedef const std::vector<ModelPtr>& ModelsConstRef;
  typedef std::vector<ModelPtr>& ModelsRef;

  //static const MaterialPtr default_material;

  Scene() = default;
  Scene(Name name) : name(name) {}

  // Destructor
  virtual ~Scene() {};

  void add_model(Model&& mi) { models.emplace_back(std::make_shared<Model>(mi)); }
  void add_model(const Mat4& trans, GeometryPtr geometry, const Name& name) {
    ModelPtr new_model = std::make_shared<Model>(name, trans, geometry, nullptr);
    models.emplace_back(new_model);
  }

  // TODO convert to iterator
  virtual ModelsConstRef get_models() const { return models; }
  virtual ModelsRef get_models() { return models; }

  // Debug info
  virtual std::wstring summary() const { return L"Base Scene"; }

  friend std::wostream& operator<<(std::wostream& os, const Scene& s) { return os << s.summary(); }

protected:
  using ModelContainer = std::vector<ModelPtr>;

  Name name = L"Scene-Un-Named";
  ModelContainer models;

  SceneHandle rendering_handle;
};

} // namespace REI

#endif
