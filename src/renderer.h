#ifndef REI_RENDERER_H
#define REI_RENDERER_H

#include <cstddef>
#include <memory>

#if WIN32
#include <windows.h>
#endif

#include "container_utils.h"
#include "type_utils.h"

#include "camera.h"
#include "color.h"
#include "scene.h"

#include "graphic_handle.h"
#include "shader_struct.h"

/*
 * renderer.h
 * Define the Renderer interface. Implementation is platform-specific.
 * (see opengl/renderer.h and d3d/renderer.h)
 */

namespace rei {

class Geometry;
class Model;
class Scene;

struct SystemWindowID {
  enum Platform {
    Win,
    Offscreen,
  } platform;
  union {
    HWND hwnd;
  } value;
};

} // namespace rei

namespace std {

/*
 *hasher and equalizer for system window id
 */

template <>
struct hash<rei::SystemWindowID> {
  using T = typename rei::SystemWindowID;
  using Platform = typename rei::SystemWindowID::Platform;
  std::size_t operator()(const rei::SystemWindowID& id) const {
    switch (id.platform) {
      case Platform::Win:
        return size_t(id.value.hwnd);
      default:
        REI_ASSERT("Unhandled platform case");
        return 0;
    }
  }
};

template <>
struct equal_to<rei::SystemWindowID> {
  using T = typename rei::SystemWindowID;
  using Platform = typename rei::SystemWindowID::Platform;
  bool operator()(const T& lhs, const T& rhs) const {
    if (lhs.platform != rhs.platform) return false;
    switch (lhs.platform) {
      case Platform::Win:
        return lhs.value.hwnd == rhs.value.hwnd;
      default:
        REI_ASSERT("Unhandled platform case");
        return false;
    }
  }
};

} // namespace std

namespace rei {

// Pramater types
// NOTE: each represent a range type in descriptor table
struct ConstantBuffer {};
struct ShaderResource {};
struct UnorderedAccess {};
struct Sampler {};
struct StaticSampler {};

// Represent a descriptor table / descriptor set, in a defined space
struct ShaderParameter {
  // TODO make this inplace memory
  // index as implicit shader register
  FixedVec<ConstantBuffer, 8> const_buffers;
  FixedVec<ShaderResource, 8> shader_resources;
  FixedVec<UnorderedAccess, 8> unordered_accesses;
  FixedVec<Sampler, 8> samplers;
  FixedVec<StaticSampler, 8> static_samplers;
};

// Represent the set of shader resources to be bound by a list of shader arguments
struct ShaderSignature {
  // index as implicit register space
  // NOTE: you dont usually use more than 4 space, right?
  FixedVec<ShaderParameter, 4> param_table;
};

struct ShaderCompileConfig {
  struct Macro {
    std::string name;
    std::string definition;
    Macro() = default;
    Macro(std::string n, std::string d = "TRUE") : name(n), definition(d) {}
  };
  FixedVec<Macro, 16> definitions;
  template<unsigned int N>
  static ShaderCompileConfig defines(FixedVec<Macro, N>&& defs) { 
    static_assert(N <= 16);
    ShaderCompileConfig ret {defs};
    return ret;
  }
};

struct ShaderArgumentValue {
  constexpr static size_t c_buf_max = 8;
  // TODO make this inplace memory
  FixedVec<BufferHandle, c_buf_max> const_buffers;
  FixedVec<size_t, c_buf_max> const_buffer_offsets;
  FixedVec<BufferHandle, c_buf_max> shader_resources;
  FixedVec<BufferHandle, c_buf_max> unordered_accesses;
  // std::vector<void*> samplers;

  size_t total_buffer_count() const {
    return const_buffers.size() + shader_resources.size() + unordered_accesses.size();
  }
};

struct RenderTargetDesc {
  ResourceFormat format = ResourceFormat::B8G8R8A8_UNORM;
};

struct RasterizationShaderMetaInfo {
  ShaderSignature signature {};
  FixedVec<RenderTargetDesc, 8> render_target_descs {RenderTargetDesc()};
  bool is_depth_stencil_disabled = false;
  bool is_blending_addictive = false;
};

struct ComputeShaderMetaInfo {
  ShaderSignature signature {};
};

// Shader info for the entire raytracing pipeline
struct RaytracingShaderMetaInfo {
  ShaderSignature global_signature;
  ShaderSignature raygen_signature;
  ShaderSignature hitgroup_signature;
  ShaderSignature miss_signature;
  std::wstring raygen_name;
  std::wstring hitgroup_name;
  std::wstring closest_hit_name;
  // std::wstring any_hit_name;
  // std::wstring intersection_name;
  std::wstring miss_name;
};

// TODO make this type safe
struct BufferDesc {
  ResourceFormat format = ResourceFormat::Undefined;
  ResourceDimension dimension = ResourceDimension::Undefined;
  size_t width = 1;
  size_t height = 1;
};

struct TextureDesc {
  size_t width;
  size_t height;
  ResourceFormat format;
  struct Flags {
    bool allow_render_target : 1;
    bool allow_depth_stencil : 1;
    bool allow_unordered_access : 1;
  } flags;

  static TextureDesc render_target(size_t width, size_t height, ResourceFormat format) {
    return {width, height, format, {true, false, false}};
  }
  static TextureDesc depth_stencil(
    size_t width, size_t height, ResourceFormat format = ResourceFormat::D24_UNORM_S8_UINT) {
    return {width, height, format, {false, true, false}};
  }
  static TextureDesc unorder_access(size_t width, size_t height, ResourceFormat format) {
    return {width, height, format, {false, false, true}};
  }
};

struct GeometryDesc {
  GeometryPtr geometry;
};

struct GeometryBuffers {
  BufferHandle vertex_buffer;
  BufferHandle index_buffer;
  BufferHandle blas_buffer;
};

struct RaytraceSceneDesc {
  std::vector<BufferHandle> blas_buffer;
  std::vector<Mat4> transform;
  std::vector<uint32_t> instance_id;
};

using ShaderArguments = FixedVec<ShaderArgumentHandle, 8>;

struct UpdateShaderTable {
  enum class TableType {
    Raygen,
    Hitgroup,
    Miss,
  } const table_type;
  ShaderHandle shader;
  BufferHandle shader_table;
  size_t index;
  ShaderArguments arguments;

  static UpdateShaderTable hitgroup() { return {TableType::Hitgroup}; }
};

struct DrawCommand {
  BufferHandle vertex_buffer = c_empty_handle;
  BufferHandle index_buffer = c_empty_handle;
  ShaderHandle shader = c_empty_handle;
  ShaderArguments arguments;
};

struct DispatchCommand {
  ShaderHandle compute_shader;
  ShaderArguments arguments;
  uint32_t dispatch_x;
  uint32_t dispatch_y;
  uint32_t dispatch_z = 1;
};

struct RaytraceCommand {
  ShaderHandle raytrace_shader;
  ShaderArguments arguments;
  BufferHandle shader_table;
  uint32_t width;
  uint32_t height;
  uint32_t depth = 1;
};

template<typename Numeric>
struct RenderRect {
  Numeric offset_left = 0;
  Numeric offset_top = 0;
  Numeric width = 0;
  Numeric height = 0;

  RenderRect shrink_to_upper_left(
    Numeric swidth, Numeric sheight, Numeric pad_left = 0, Numeric pad_top = 0) const {
    REI_ASSERT(height >= sheight + pad_top);
    return {offset_left + pad_left, offset_top + pad_top, swidth, sheight};
  }

  RenderRect shrink_to_lower_left(
    Numeric swidth, Numeric sheight, Numeric pad_left = 0, Numeric pad_bottom = 0) const {
    REI_ASSERT(height >= sheight + pad_bottom);
    return {offset_left + pad_left, offset_top + height - (pad_bottom + sheight), swidth, sheight};
  }

  static RenderRect full(Numeric width, Numeric height) {
    return {0, 0, Numeric(width), Numeric(height)};
  }
};

using RenderViewaport = RenderRect<float>;
using RenderArea = RenderRect<int>;

struct RenderPassCommand {
  FixedVec<BufferHandle, 8> render_targets;
  BufferHandle depth_stencil;
  RenderViewaport viewport;
  RenderArea area;
  bool clear_rt;
  bool clear_ds;
};

class Renderer : private NoCopy {
public:
  Renderer() {}
  virtual ~Renderer() {}

  virtual bool is_depth_range_01() const = 0;

protected:
  // Convert from handle to data
  template <typename Handle, typename Data>
  std::shared_ptr<Data> get_data(Handle handle) {
    if (handle && (handle->owner == this)) {
      // Pointer to forward-delcared class cannot be static up-cast, so we have to use reinterpret
      // cast
      return std::static_pointer_cast<Data>(handle);
      // return std::reinterpret_pointer_cast<Data>(handle);
    }
    return nullptr;
  }
};

} // namespace rei

#endif
