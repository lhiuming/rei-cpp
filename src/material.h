#ifndef REI_MATERIAL_H
#define REI_MATERIAL_H

#include <memory>

#include "color.h"
#include "common.h"
#include "container_utils.h"
#include "string_utils.h"
#include "variant_utils.h"

namespace rei {

// fwd decl
class Materials;

/*
 * General material property data.
 * Basically a bunch of hashmaps, storing limited amount of data types.
 */
class Material : NoCopy {
public:
  using ID = std::uintptr_t;
  static constexpr ID kInvalidID = 0;

  // clang-format off
  using MaterialProperty = Var<
    std::monostate,
    Color, 
    Vec4, 
    double
    // Texture
    >;
  // clang-format on

  ID id() const { return m_id; }

  bool has(const Name& prop_name) const { return m_properties.has(prop_name); }

  void set(Name&& prop_name, MaterialProperty&& prop_value) {
    m_properties.insert_or_assign(std::move(prop_name), std::move(prop_value));
  }

  const MaterialProperty& get(const Name& prop_name) const {
    const MaterialProperty* value = m_properties.try_get(prop_name);
    return value ? *value : MaterialProperty();
  }

  template <typename T>
  std::optional<T> get(const Name& prop_name) const {
    const MaterialProperty* value = m_properties.try_get(prop_name);
    const T* ptr = std::get_if<T>(value);
    return ptr ? *ptr : std::optional<T>();
  }

  // void set_graphic_handle(MaterialHandle h) { graphic_handle = h; }
  // MaterialHandle get_graphic_handle() const { return graphic_handle; }

private:
  ID m_id;
  Name m_name;
  Hashmap<Name, MaterialProperty> m_properties;

  friend Materials;

  Material(ID id) : m_id(id), m_name(L"Unnamed") {}
  Material(ID id, Name&& name) : m_id(id), m_name(std::move(name)) {}

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

class Materials {
public:
  Materials();
  ~Materials() = default;

  MaterialPtr create(Name name);
  void destroy(MaterialPtr mat);

private:
  Material::ID m_next_id;
  Hashmap<Material::ID, MaterialPtr> m_materials;
};

} // namespace rei

#endif