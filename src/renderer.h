#ifndef REI_RENDERER_H
#define REI_RENDERER_H

#include <cstddef>
#include <memory>

#if WIN32
#include <windows.h>
#endif

#include "camera.h"
#include "color.h"
#include "container_utils.h"
#include "graphic_handle.h"
#include "shader_struct.h"
#include "type_utils.h"

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

template<>
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

template<>
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
struct ConstBuffer {};
struct ShaderResource {};
struct UnorderedAccess {};
struct Sampler {};

// Represent a descriptor table / descriptor set, in a defined space
struct ShaderParameter {
  // TODO make this inplace memory
  // index as implicit shader register
  std::vector<ConstBuffer> const_buffers;
  std::vector<ShaderResource> shader_resources;
  std::vector<UnorderedAccess> unordered_accesses;
  std::vector<Sampler> samplers;
};

// Represent the set of shader resources to be bound by a list of shader arguments
struct ShaderSignature {
  // index as implicit register space
  std::vector<ShaderParameter> param_table;
  // TODO  support static samplers
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

struct RasterizationShaderMetaInfo {
  ShaderSignature signature;
  bool is_depth_stencil_disabled = false;
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
  //std::wstring any_hit_name;
  //std::wstring intersection_name;
  std::wstring miss_name;
};

using ShaderArguments = FixedVec<ShaderArgumentHandle, 8>;
struct DrawCommand {
  GeometryHandle geo;
  ShaderHandle shader;
  ShaderArguments arguments;
};

class Renderer : private NoCopy {
public:
  Renderer() {}
  virtual ~Renderer() {}

  virtual GeometryHandle create_geometry(const Geometry& geo) = 0;
  virtual ModelHandle create_model(const Model& model) = 0;

  virtual bool is_depth_range_01() const = 0;

protected:
  // Convert from handle to data
  template <typename Handle, typename Data>
  std::shared_ptr<Data> get_data(Handle handle) {
    if (handle && (handle->owner == this)) {
      // Pointer to forward-delcared class cannot be static up-cast, so we have to use reinterpret
      // cast
      return std::static_pointer_cast<Data>(handle);
      //return std::reinterpret_pointer_cast<Data>(handle);
    }
    return nullptr;
  }
};

} // namespace rei

#endif
