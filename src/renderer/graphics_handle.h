#ifndef REI_GRAPHIC_HANDLE_H
#define REI_GRAPHIC_HANDLE_H

#include "graphics_const.h"

/*
 * Some pointer/ID type for communicating between renderer and scenen objects.
 */

namespace rei {

class Renderer;


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
