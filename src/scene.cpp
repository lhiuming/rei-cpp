// source of scene.h
#include "scene.h"

#include <memory>
#include <sstream>
#include <vector>

#include "math/algebra.h"

namespace rei {

/*
wstring StaticScene::summary() const {
wostringstream oss;
oss << "Scene name: " << name << ", with " << models.size() << " models" << endl;
for (const auto& mi : models) {
  oss << "  " << *(mi.pmodel);
  oss << "  with trans: " << mi.transform << endl;
}
return oss.str();
}
*/

std::wstring Model::summary() const {
  return m_name;
}

Model::Model(ID id, Name&& n) : Model(id, std::move(n), Mat4::I(), nullptr, nullptr) {}

Model::Model(ID id, Name&& n, Mat4 trans, GeometryPtr geometry, MaterialPtr material)
    : m_id(id),
      m_name(std::move(n)),
      m_transform(trans),
      m_geometry(geometry),
      m_material(material) {}

Scene::Scene(Name name) : m_name(name), m_next_id(1) {}

ModelPtr Scene::create_model(
  const Mat4& trans, GeometryPtr geometry, MaterialPtr material, Name&& name) {
  REI_ASSERT(m_next_id != Model::kInvalidID);
  auto ptr = std::shared_ptr<Model>(new Model(m_next_id, std::move(name), trans, geometry, material));
  m_models.push_back(ptr);
  m_next_id++;
  return ptr;
}

} // namespace rei
