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
  using Property = Var<
    std::monostate,
    Color, 
    Vec4, 
    double
    // Texture
    >;
  // clang-format on

  class PropertySet {
  public:
    bool has(const Name& prop_name) const { return m_table.has(prop_name); }

    void set(Name&& prop_name, Property&& prop_value) {
      m_table.insert_or_assign(std::move(prop_name), std::move(prop_value));
    }

    const Property& get(const Name& prop_name) const {
      const Property* value = m_table.try_get(prop_name);
      return value ? *value : Property();
    }

    template <typename T>
    std::optional<T> get(const Name& prop_name) const {
      const Property* value = m_table.try_get(prop_name);
      const T* ptr = std::get_if<T>(value);
      return ptr ? *ptr : std::optional<T>();
    }

  private:
    Hashmap<Name, Property> m_table;
    friend Material;

    friend std::wostream& operator<<(std::wostream& os, const PropertySet& props) {
      os << "{";
      auto beg = props.m_table.cbegin();
      const auto end = props.m_table.cend();
      while (beg != end) {
        os << beg->first << ":" << beg->second;
        beg++;
        if (beg != end) { os << ", "; }
      }
      os << "}";
      return os;
    }

  };

  ID id() const { return m_id; }

  // Proxy methods
  bool has(const Name& prop_name) const { return m_props.has(prop_name); }
  const Property& get(const Name& prop_name) const { return m_props.get(prop_name); }
  template <typename T>
  std::optional<T> get(const Name& prop_name) const {
    return m_props.get<T>(prop_name);
  }
  void set(Name&& prop_name, Property&& prop_value) {
    m_props.set(std::move(prop_name), std::move(prop_value));
  }

private:
  ID m_id;
  Name m_name;
  PropertySet m_props;

  friend Materials;

  Material(ID id) : m_id(id), m_name(L"Unnamed") {}
  Material(ID id, Name&& name) : m_id(id), m_name(std::move(name)) {}

  friend std::wostream& operator<<(std::wostream& os, const Material& mat);
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