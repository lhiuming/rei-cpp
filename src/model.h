#ifndef REI_MODEL_H
#define REI_MODEL_H

#include <memory>
#include <vector>

#include "algebra.h"
#include "color.h"
#include "geometry.h"
#include "graphic_handle.h"

/*
 * model.h
 * This module considers how a model is composed.
 *
 * NOTE: A Model object should be unique, but possible to have multiple
 * "instances" by relating with different transforms. (see scene.h)
 *
 * TODO: add aggregated model (and animated model)
 * TODO: Support cube and sphere
 */

namespace rei {

// Simple Material class
class Material {
public:
  std::string name = "REI_Default";
  Color diffuse = {0.2, 0.2, 0.2, 1.0};
  Color ambient = {0.2, 0.2, 0.2, 1.0};
  Color specular = {0.2, 0.2, 0.2, 1.0};
  double shineness = 30;

  //void set_graphic_handle(MaterialHandle h) { graphic_handle = h; }
  //MaterialHandle get_graphic_handle() const { return graphic_handle; }

  friend std::wostream& operator<<(std::wostream& os, Material mat) {
    return os << "name = " << mat.name << ", diff = " << mat.diffuse << ", ambi = " << mat.ambient;
  }

protected:
  //MaterialHandle graphic_handle;

};

typedef std::shared_ptr<Material> MaterialPtr;


// Model classes //////////////////////////////////////////////////////////////

// Base class
// supports polymorphism only through RTTI
class Model {
public:
  // Default construct
  [[depreacated]]
  Model(const std::string& n) { REI_DEPRECATED }
  Model(const std::wstring& n) : Model(n, Mat4::I(), nullptr, nullptr) {}
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
  virtual std::wstring summary() const;
  friend std::wostream& operator<<(std::wostream& os, const Model& m) { return os << m.summary(); }

protected:
  Name name = L"Model-Un-Named";
  Mat4 transform = Mat4::I();
  GeometryPtr geometry;
  MaterialPtr material;

  ModelHandle rendering_handle;
};

typedef std::shared_ptr<Model> ModelPtr;

// Model Aggregates
class Aggregate : public Model {
  // Dump classs
};

} // namespace REI

#endif
