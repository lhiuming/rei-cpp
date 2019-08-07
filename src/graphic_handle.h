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
  R32G32B32A32Float,
  R32G32Float,
  R8G8B8A8Unorm,
  D24Unorm_S8Uint,
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
  Undefined,
  Present,
  RenderTarget,
  DeptpWrite,
  CopySource,
  CopyDestination,
  PixelShaderResource,
  ComputeShaderResource,
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

// TODO remove this
using ScreenTransformHandle = std::shared_ptr<BaseScreenTransformData>;
using SwapchainHandle = std::shared_ptr<BaseSwapchainData>;
using BufferHandle = std::shared_ptr<BaseBufferData>;
using ShaderArgumentHandle = std::shared_ptr<BaseShaderArgument>;

using ShaderHandle = std::shared_ptr<BaseShaderData>;

inline static constexpr decltype(nullptr) c_empty_handle = nullptr;

} // namespace rei

#endif
