#ifndef REI_SCENE_H
#define REI_SCENE_H

#include <memory>
#include <set>

#include "math/algebra.h"
#include "geometry.h"
#include "material.h"
#include "renderer/graphics_handle.h"

/*
 * scene.h
 * Define a Scene class
 *
 * TODO: add lights
 * TODO: add audio
 */

namespace rei {

// fwd decl
class Scene;

/*
 * Represent a object to be render in the scene.
 * Conceptually, it is a composition of geometry, material and transform.
 */
class Model : NoCopy {
public:
  using ID = std::uintptr_t;
  static constexpr ID kInvalidID = 0;

  void set_transform(const Mat4 trans) { this->m_transform = trans; }
  Mat4 get_transform(Handness from = Handness::Right, Handness to = Handness::Right,
    VectorTarget vec = VectorTarget::Column) const {
    return convention_convert(
      m_transform, from != Handness::Right, to != Handness::Right, vec != VectorTarget::Column);
  }

  ID id() const { return m_id; }

  void set_material(std::shared_ptr<Material> mat) { this->m_material = mat; }
  MaterialPtr get_material() const { return m_material; }

  void set_geometry(std::shared_ptr<Geometry> geo) { this->m_geometry = geo; }
  GeometryPtr get_geometry() const { return m_geometry; }

  // Debug info
  std::wstring summary() const;
  friend std::wostream& operator<<(std::wostream& os, const Model& m) { return os << m.summary(); }

private:
  ID m_id;
  Name m_name = L"<Model Unnamed>";
  Mat4 m_transform = Mat4::I();
  GeometryPtr m_geometry;
  MaterialPtr m_material;

  friend Scene;

  Model(ID id, Name&& n);
  Model(ID id, Name&& n, Mat4 trans, GeometryPtr geometry, MaterialPtr material);
};

using ModelPtr = std::shared_ptr<Model>;

/*
 * Describe a 3D environment to be rendered.
 * Typically, it contains: models, lights, ...
 */
class Scene {
  using ModelContainer = std::vector<ModelPtr>;

public:
  // Member types
  typedef const std::vector<ModelPtr>& ModelsConstRef;
  typedef std::vector<ModelPtr>& ModelsRef;

  typedef uintptr_t ModelUID;

  // static const MaterialPtr default_material;

  Scene(Name name = L"New Scene");

  ModelPtr create_model(const Mat4& trans, GeometryPtr geometry, MaterialPtr material, Name&& name);
  ModelPtr create_model(const Mat4& trans, GeometryPtr geometry, Name&& name) {
    return create_model(trans, geometry, nullptr, std::move(name));
  }

  // TODO convert to iterator
  virtual ModelsConstRef get_models() const { return m_models; }
  virtual ModelsRef get_models() { return m_models; }

  // Debug info
  virtual std::wstring summary() const { return L"Base Scene"; }

  friend std::wostream& operator<<(std::wostream& os, const Scene& s) { return os << s.summary(); }

protected:
  Name m_name;
  Model::ID m_next_id;
  ModelContainer m_models;
};

} // namespace rei

#endif
