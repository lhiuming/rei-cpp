#ifndef REI_SCENE_H
#define REI_SCENE_H

#include <memory>
#include <set>

#include "algebra.h"
#include "renderer/graphics_handle.h"

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
  Model(const std::string& n) {REI_DEPRECATED} 
  Model(const std::wstring& n)
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
};

using ModelPtr = std::shared_ptr<Model>;

/*
 * Describe a 3D environment to be rendered.
 * Typically, it contains: models, lights, ...
 */
class Scene {
  using ModelContainer = std::vector<ModelPtr>;
  using MaterialContainer = std::set<MaterialPtr>;
  using GeometryContainer = std::set<GeometryPtr>;

public:
  // Member types
  typedef const std::vector<ModelPtr>& ModelsConstRef;
  typedef std::vector<ModelPtr>& ModelsRef;

  typedef uintptr_t MaterialUID;
  typedef uintptr_t GeometryUID;
  typedef uintptr_t ModelUID;

  // static const MaterialPtr default_material;

  Scene() = default;
  Scene(Name name) : name(name) {}

  // Destructor
  virtual ~Scene() {};

  void add_model(const Mat4& trans, GeometryPtr geometry, MaterialPtr material, const Name& name) {
    ModelPtr new_model = std::make_shared<Model>(name, trans, geometry, material);
    if (material) m_materials.insert(material);
    if (geometry) m_geometries.insert(geometry);
    m_models.emplace_back(new_model);
  }
  void add_model(const Mat4& trans, GeometryPtr geometry, const Name& name) {
    add_model(trans, geometry, nullptr, name);
  }
  void add_model(Model&& mi) {
    auto mat = mi.get_material();
    if (mat) m_materials.insert(mat);
    auto geo = mi.get_geometry();
    if (geo) m_geometries.insert(geo);
    m_models.emplace_back(std::make_shared<Model>(mi));
  }

  // TODO convert to iterator
  virtual ModelsConstRef get_models() const { return m_models; }
  virtual ModelsRef get_models() { return m_models; }

  // Just use the address as runtime id, for convinient
  inline GeometryUID get_id(const GeometryPtr& geometry) const { return uintptr_t(geometry.get()); }
  inline MaterialUID get_id(const MaterialPtr& material) const { return uintptr_t(material.get()); }
  inline ModelUID get_id(const ModelPtr& model) const { return uintptr_t(model.get()); }

  const MaterialContainer& materials() const { return m_materials; }
  const GeometryContainer& geometries() const { return m_geometries; }

  // Debug info
  virtual std::wstring summary() const { return L"Base Scene"; }

  friend std::wostream& operator<<(std::wostream& os, const Scene& s) { return os << s.summary(); }

protected:
  Name name = L"Scene-Un-Named";
  ModelContainer m_models;
  MaterialContainer m_materials;
  GeometryContainer m_geometries;
};

} // namespace rei

#endif
