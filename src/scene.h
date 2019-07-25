#ifndef REI_SCENE_H
#define REI_SCENE_H

#include <memory>

#include "algebra.h"
#include "graphic_handle.h"

#include "geometry.h"
#include "material.h"

/*
 * scene.h
 * Define a Scene class
 *
 * TODO: add lights
 * TODO: add audio
 */

namespace rei {

/*
 * Represent a object to be render in the scene.
 * Conceptually, it is a composition of geometry, material and transform.
 */
class Model {
public:
  // Default construct
  [[depreacated]] Model(const std::string& n) {REI_DEPRECATED} Model(const std::wstring& n)
      : Model(n, Mat4::I(), nullptr, nullptr) {}
  Model(const std::wstring& n, Mat4 trans, GeometryPtr geometry, MaterialPtr material)
      : name(n), transform(trans), geometry(geometry), material(material) {}

  // Destructor
  virtual ~Model() = default;

  void set_transform(const Mat4 trans) { this->transform = trans; }
  Mat4 get_transform(Handness from = Handness::Right, Handness to = Handness::Right,
    VectorTarget vec = VectorTarget::Column) const {
    return convention_convert(
      transform, from != Handness::Right, to != Handness::Right, vec != VectorTarget::Column);
  }

  void set_material(std::shared_ptr<Material> mat) { this->material = mat; }
  MaterialPtr get_material() const { return material; }

  void set_geometry(std::shared_ptr<Geometry> geo) { this->geometry = geo; }
  GeometryPtr get_geometry() const { return geometry; }

  void set_rendering_handle(ModelHandle h) { rendering_handle = h; }
  ModelHandle get_rendering_handle() const { return rendering_handle; }

  [[deprecated]] void set(const Material& mat) { REI_DEPRECATED }
  [[deprecated]] void set(Material&& mat) { REI_DEPRECATED }

  // Debug info
  virtual std::wstring summary() const { return L""; }
  friend std::wostream& operator<<(std::wostream& os, const Model& m) { return os << m.summary(); }

protected:
  Name name = L"Model-Un-Named";
  Mat4 transform = Mat4::I();
  GeometryPtr geometry;
  MaterialPtr material;

  ModelHandle rendering_handle;
};

using ModelPtr = std::shared_ptr<Model>;

/*
 * Describe a 3D environment to be rendered.
 * Typically, it contains: models, lights, ...
 */
class Scene {
public:
  // Member types
  typedef const std::vector<ModelPtr>& ModelsConstRef;
  typedef std::vector<ModelPtr>& ModelsRef;

  // static const MaterialPtr default_material;

  Scene() = default;
  Scene(Name name) : name(name) {}

  // Destructor
  virtual ~Scene() {};

  void add_model(const Mat4& trans, GeometryPtr geometry, MaterialPtr material, const Name& name) {
    ModelPtr new_model = std::make_shared<Model>(name, trans, geometry, material);
    models.emplace_back(new_model);
  }
  void add_model(const Mat4& trans, GeometryPtr geometry, const Name& name) {
    add_model(trans, geometry, nullptr, name);
  }
  void add_model(Model&& mi) { models.emplace_back(std::make_shared<Model>(mi)); }

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
};

} // namespace rei

#endif
