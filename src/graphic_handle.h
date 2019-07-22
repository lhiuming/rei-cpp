#ifndef REI_GRAPHIC_HANDLE_H
#define REI_GRAPHIC_HANDLE_H

#include "algebra.h"

/*
 * Some pointer/ID type for communicating between renderer and scenen objects.
 */

namespace rei {

class Renderer;

enum class ResourceFormat {
  Undefined,
  R32G32B32A32_FLOAT,
  B8G8R8A8_UNORM,
  D24_UNORM_S8_UINT,
  AcclerationStructure,
  Count
};

enum class ResourceDimension {
  Undefined,
  Raw,
  StructuredBuffer,
  Texture1D,
  Texture1DArray,
  Texture2D,
  Texture2DArray,
  Texture3D,
  AccelerationStructure,
};

enum class ResourceState {
  Present,
  RenderTarget,
  DeptpWrite,
  UnorderedAccess,
};

struct GraphicData {
  GraphicData(Renderer* owner) : owner(owner) {}
  const Renderer const* owner; // as a marker
};

struct BaseScreenTransformData : GraphicData {
  using GraphicData::GraphicData;
  bool enable_vsync = false;
};

struct BaseSwapchainData : GraphicData {
  using GraphicData::GraphicData;
};

struct BaseBufferData : GraphicData {
  using GraphicData::GraphicData;
};

struct BaseShaderData : GraphicData {
  using GraphicData::GraphicData;
};

struct BaseShaderArgument : GraphicData {
  using GraphicData::GraphicData;
};

struct BaseModelData : GraphicData {
  using GraphicData::GraphicData;
  Mat4 transform;
};

struct BaseGeometryData : GraphicData {
  using GraphicData::GraphicData;
};

// TODO remove this
using ScreenTransformHandle = std::shared_ptr<BaseScreenTransformData>;
using SwapchainHandle = std::shared_ptr<BaseSwapchainData>;
using BufferHandle = std::shared_ptr<BaseBufferData>;
using ShaderArgumentHandle = std::shared_ptr<BaseShaderArgument>;

using ShaderHandle = std::shared_ptr<BaseShaderData>;
using GeometryHandle = std::shared_ptr<BaseGeometryData>;
// TODO remove this
using ModelHandle = std::shared_ptr<BaseModelData>;

inline static constexpr decltype(nullptr) c_empty_handle = nullptr;

} // namespace rei

#endif
