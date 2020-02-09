#include "material.h"

namespace rei {

Materials::Materials() : m_next_id(1) {}

MaterialPtr Materials::create(Name name) {
  REI_ASSERT(m_next_id != Material::kInvalidID);
  MaterialPtr ptr = std::shared_ptr<Material>(new Material(m_next_id, std::move(name)));
  m_materials.insert(ptr->id(), ptr);
  m_next_id += 1;
  return ptr;
}

void Materials::destroy(MaterialPtr mat) {
  Material::ID k = mat->id();
  REI_ASSERT(k != Material::kInvalidID);
  int count = m_materials.erase(k);
  REI_ASSERT(count == 1, format("Destory material with %d instance!", count));
}

}
