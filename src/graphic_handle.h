#ifndef REI_GRAPHIC_HANDLE_H
#define REI_GRAPHIC_HANDLE_H

#include "algebra.h"

/*
 * Some pointer/ID type for communicating between renderer and scenen objects.
 */

namespace rei {

class Renderer;

struct GraphicData {
  GraphicData(Renderer* owner) : owner(owner) {}
  const Renderer const* owner; // as a marker
};

struct BaseViewportData : GraphicData {
  using GraphicData::GraphicData;
  bool enable_vsync = false;
};

struct BaseShaderData : GraphicData {
  using GraphicData::GraphicData;
};

struct BaseMaterialData : GraphicData {
  using GraphicData::GraphicData;
};

struct BaseModelData : GraphicData {
  using GraphicData::GraphicData;
  Mat4 transform;
};

struct BaseGeometryData : GraphicData {
  using GraphicData::GraphicData;
};

struct BaseSceneData: GraphicData {
  using GraphicData::GraphicData;
};

struct BaseCullingData : GraphicData {
  using GraphicData::GraphicData;
};

using ViewportHandle = std::shared_ptr<BaseViewportData>;
using ShaderHandle = std::shared_ptr<BaseShaderData>;
using GeometryHandle = std::shared_ptr<BaseGeometryData>;
using MaterialHandle = std::shared_ptr<BaseMaterialData>;
using ModelHandle = std::shared_ptr<BaseModelData>;
using SceneHandle = std::shared_ptr<BaseSceneData>;
using CullingResult = std::shared_ptr<BaseCullingData>;

} // namespace rei

#endif
