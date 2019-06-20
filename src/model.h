#ifndef REI_MODEL_H
#define REI_MODEL_H

#include <memory>
#include <vector>

#include "algebra.h"
#include "color.h"
#include "geometry.h"

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

// Model classes //////////////////////////////////////////////////////////////

// Base class
// supports polymorphism only through RTTI
class Model {
public:
  // Simple Material class
  struct Material {
    std::string name = "REI_Default";
    Color diffuse = {0.2, 0.2, 0.2, 1.0};
    Color ambient = {0.2, 0.2, 0.2, 1.0};
    Color specular = {0.2, 0.2, 0.2, 1.0};
    double shineness = 30;

    friend std::wostream& operator<<(std::wostream& os, Material mat) {
      return os << "name = " << mat.name << ", diff = " << mat.diffuse
                << ", ambi = " << mat.ambient;
    }
  };

public:
  // Public data
  std::string name = "Model Un-named";

public:
  // Default construct
  Model(std::string n) : name(n) {}

  // Destructor
  virtual ~Model() = default;

  void set_material(std::shared_ptr<Material> mat) { this->material = mat; }
  std::shared_ptr<Material> get_material() { return material; }

  void set_geometry(std::shared_ptr<Geometry> geo) { this->geometry = geo; }
  std::shared_ptr<Geometry> get_geometry() { return geometry; }

  [[deprecated]] void set(const Material& mat) { DEPRECATED }
  [[deprecated]] void set(Material&& mat) { DEPRECATED }
  [[deprecated]] const Material& get_material() const { DEPRECATED }

  // Debug info
  virtual std::wstring summary() const;
  friend std::wostream& operator<<(std::wostream& os, const Model& m) { return os << m.summary(); }

protected:
  std::shared_ptr<Geometry> geometry;
  std::shared_ptr<Material> material;
};

// Model Aggregates
class Aggregate : public Model {
  // Dump classs
};

} // namespace REI

#endif
