#ifndef REI_MATERIAL_H
#define REI_MATERIAL_H

#include <memory>

#include "string_utils.h"
#include "variant_utils.h"
#include "container_utils.h"

#include "color.h"

namespace rei {

/*
 * General material property data. 
 * Basically a bunch of hashmaps, storing limited amount of data types.
 */
class Material {
public:
  // clang-format off
  using MaterialProperty = Var<
    std::monostate,
    Color, 
    Vec4, 
    double
    // Texture
    >;
  // clang-format on

  Material() : m_name(L"Unnamed") {}
  Material(Name&& name) : m_name(name) {}

  Material(Material&& other) = default;
  Material(const Material& other) = default;

  bool has(const Name& prop_name) const { return m_properties.has(prop_name); }

  void set(Name&& prop_name, MaterialProperty&& prop_value) {
    m_properties.insert_or_assign(std::move(prop_name), std::move(prop_value));
  }

  const MaterialProperty& get(const Name& prop_name) const {
    const MaterialProperty* value = m_properties.try_get(prop_name);
    return value ? *value : MaterialProperty();
  }

  // void set_graphic_handle(MaterialHandle h) { graphic_handle = h; }
  // MaterialHandle get_graphic_handle() const { return graphic_handle; }

protected:
  Name m_name;
  Hashmap<Name, MaterialProperty> m_properties;

  friend std::wostream& operator<<(std::wostream& os, const Material& mat) {
    os << mat.m_name << " {";
    auto beg = mat.m_properties.cbegin();
    const auto end = mat.m_properties.cend();
    while (beg != end) {
      os << beg->first << ":" << beg->second;
      beg++;
      if (beg != end) { os << ", "; }
    }
    os << "}";
  }
};

using MaterialPtr = std::shared_ptr<Material>;

} // namespace rei

#endif