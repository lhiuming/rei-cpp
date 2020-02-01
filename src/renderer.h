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
#include "shader_struct.h"
/*
 * renderer.h
 */

#include "renderer/graphics_handle.h"
#include "renderer/graphics_desc.h"

//#include "direct3d/d3d_resource_types.h"
#include "direct3d/d3d_common_resources.h"
#include "direct3d/d3d_renderer_resources.h"

namespace rei {

class Geometry;
class Model;
class Scene;

struct SystemWindowID {
  enum class Platform {
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

struct RenderImpl;

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
  ShaderSignature signature {};
  FixedVec<RenderTargetDesc, 8> render_target_descs {RenderTargetDesc()};
  FixedVec<VertexInputDesc, 8> vertex_input_desc;
  MergeDesc merge;
  bool is_depth_stencil_disabled = false;
  bool front_clockwise = false;
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
    size_t width, size_t height, ResourceFormat format = ResourceFormat::D24Unorm_S8Uint) {
    return {width, height, format, {false, true, false}};
  }
  static TextureDesc unorder_access(size_t width, size_t height, ResourceFormat format) {
    return {width, height, format, {false, false, true}};
  }
  static TextureDesc simple_2d(size_t width, size_t height, ResourceFormat format) {
    return { width, height, format, {false, false, false} };
  }
};

struct GeometryFlags {
  bool dynamic : 1;
  bool include_blas : 1;
};

struct LowLevelGeometryData {
  const void* addr = nullptr;
  size_t element_count = 0;
  size_t element_bytesize = 0;
};

struct LowLevelGeometryDesc {
  LowLevelGeometryData index {};
  LowLevelGeometryData vertex {};
  GeometryFlags flags {};
};

struct GeometryDesc {
  GeometryPtr geometry;
  GeometryFlags flags {};
};

struct GeometryBufferHandles {
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

template <typename Numeric>
struct RenderRect {
  Numeric offset_left = 0;
  Numeric offset_top = 0;
  Numeric width = 0;
  Numeric height = 0;

  bool is_empty() const { return width <= 0 || height <= 0; }

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
  BufferHandle depth_stencil = c_empty_handle;
  RenderViewaport viewport;
  RenderArea area;
  bool clear_rt;
  bool clear_ds;
};

struct DrawCommand {
  BufferHandle vertex_buffer = c_empty_handle;
  BufferHandle index_buffer = c_empty_handle;
  ShaderHandle shader = c_empty_handle;
  ShaderArguments arguments {};
  size_t index_count = SIZE_MAX;
  size_t index_offset = 0;
  size_t vertex_offset = 0;
  RenderArea override_area; // optional
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

namespace d3d {
class DeviceResources;
class ViewportResources;
struct ShaderData;
struct ViewportData;
struct CullingData;

struct CommittedResource;
}

class Renderer : private NoCopy {
  using Self = typename Renderer;

  template <typename T>
  using ComPtr = typename d3d::ComPtr<T>;

  using DeviceResources = typename d3d::DeviceResources;
  using RasterizationShaderData = typename d3d::RasterizationShaderData;

public:
  struct Options {
    bool enable_realtime_raytracing = true;
  };

  Renderer(HINSTANCE hinstance, Options options = {});
  ~Renderer() = default;

  SwapchainHandle create_swapchain(
    SystemWindowID window_id, size_t width, size_t height, size_t rendertarget_count);
  BufferHandle fetch_swapchain_render_target_buffer(SwapchainHandle swapchain);
  void release_swapchain(SwapchainHandle h) {REI_NOT_IMPLEMENTED};

  BufferHandle create_texture(
    const TextureDesc& desc, ResourceState init_state, const Name& debug_name);
  void upload_texture(BufferHandle texture, const void* pixels);
  [[deprecated]] BufferHandle create_texture_2d(
    const TextureDesc& desc, ResourceState init_state, const std::wstring& debug_name) {
    return create_texture(desc, init_state, debug_name);
  }
  [[deprecated]] BufferHandle create_texture_2d(
    size_t width, size_t height, ResourceFormat format, const Name& debug_name) {
    TextureDesc desc {width, height, format};
    return create_texture_2d(desc, ResourceState::Undefined, debug_name);
  }
  [[deprecated]] BufferHandle create_unordered_access_buffer_2d(size_t width, size_t height,
    ResourceFormat format, const Name& debug_name = L"Unnamed UA Buffer") {
    return create_texture_2d(TextureDesc::unorder_access(width, height, format),
      ResourceState::UnorderedAccess, debug_name);
  }

  BufferHandle create_const_buffer(
    const ConstBufferLayout& layout, size_t num, const Name& debug_name = L"Unnamed ConstBuffer");

  void update_const_buffer(BufferHandle buffer, size_t index, size_t member, Vec4 value);
  void update_const_buffer(BufferHandle buffer, size_t index, size_t member, Mat4 value);

  ShaderHandle create_shader(const std::wstring& shader_path, RasterizationShaderMetaInfo&& meta,
    const ShaderCompileConfig& config = {});
  ShaderHandle create_shader(const char* vertex_shader, const char* index_shader,
    RasterizationShaderMetaInfo&& meta, const ShaderCompileConfig& config = {});
  [[deprecated]] ShaderHandle create_shader(const std::wstring& shader_path,
    std::unique_ptr<RasterizationShaderMetaInfo>&& meta, const ShaderCompileConfig& config = {}) {
    return create_shader(shader_path, std::move(*meta), config);
  }
  ShaderHandle finalize_shader_creation(
    ComPtr<ID3DBlob> vs, ComPtr<ID3DBlob> ps, std::shared_ptr<RasterizationShaderData> shader);

  ShaderHandle create_shader(const std::wstring& shader_path, ComputeShaderMetaInfo&& meta,
    const ShaderCompileConfig& config = {});

  ShaderHandle create_shader(const std::wstring& shader_path, RaytracingShaderMetaInfo&& meta,
    const ShaderCompileConfig& config = {});
  ShaderHandle create_raytracing_shader(const std::wstring& shader_path,
    std::unique_ptr<RaytracingShaderMetaInfo>&& meta, const ShaderCompileConfig& config = {}) {
    return create_shader(shader_path, std::move(*meta), config);
  }

  ShaderArgumentHandle create_shader_argument(ShaderArgumentValue arg_value);
  void update_shader_argument(
    ShaderHandle shader, ShaderArgumentValue arg_value, ShaderArgumentHandle& arg_handle);

  GeometryBufferHandles create_geometry(
    const LowLevelGeometryDesc& geometry, const Name& debug_name = L"Unamed Goemetry");
  GeometryBufferHandles create_geometry(const GeometryDesc& geometry);
  void update_geometry(
    BufferHandle handle, LowLevelGeometryData data, size_t dest_element_offset = 0);

  BufferHandle create_raytracing_accel_struct(
    const RaytraceSceneDesc& scene, const Name& debug_name = L"Unamed TLAS");
  BufferHandle create_shader_table(size_t intersection_count, ShaderHandle raytracing_shader);
  BufferHandle create_shader_table(const Scene& scene, ShaderHandle raytracing_shader);

  // void update_raygen_shader_record();
  void update_shader_table(const UpdateShaderTable& cmd);

  void begin_render_pass(const RenderPassCommand& cmd);
  void end_render_pass();

  void transition(BufferHandle buffer, ResourceState state);
  void barrier(BufferHandle buffer);

  void draw(const DrawCommand& cmd);

  void dispatch(const DispatchCommand& dispatch);

  void raytrace(const RaytraceCommand& cmd);
  void raytrace(ShaderHandle raytrace_shader, ShaderArguments arguments, BufferHandle shader_table,
    size_t width, size_t height, size_t depth = 1) {
    RaytraceCommand cmd {};
    cmd.raytrace_shader = raytrace_shader;
    cmd.arguments = std::move(arguments);
    cmd.shader_table = std::move(shader_table);
    cmd.width = width;
    cmd.height = height;
    cmd.depth = depth;
    raytrace(cmd);
  }

  void clear_texture(BufferHandle target, Vec4 clear_value, RenderArea clear_area);
  void copy_texture(BufferHandle src, BufferHandle dest, bool revert_state = true);

  // TODO return a command list object
  Renderer* prepare();
  void present(SwapchainHandle swapchain, bool vsync);

  DeviceResources& device() const { return *device_resources; }

  bool is_depth_range_01() const { return true; }

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

protected:
  HINSTANCE hinstance;
  const bool is_dxr_enabled;

  // some const config
  constexpr static VectorTarget model_trans_target = VectorTarget::Column;

  // TODO make unique
  std::shared_ptr<DeviceResources> device_resources;
  std::vector<d3d::CommittedResource> m_delayed_release;
  std::vector<ComPtr<ID3D12Resource>> m_delayed_release_raw;
  Hashmap<BufferHandle, UINT> m_texture_clear_descriptor;

  bool is_uploading_resources = false;

  Hashmap<const ID3D12Resource*, D3D12_CPU_DESCRIPTOR_HANDLE> m_dsv_cache {};
  Hashmap<const ID3D12Resource*, D3D12_CPU_DESCRIPTOR_HANDLE> m_rtv_cache {};

  // TODO move state to cmd_list object
  const d3d::RenderTargetSpec curr_target_spec {};

  // Ray Tracing Resources //
  UINT next_tlas_instance_id = 0;
  UINT generate_tlas_instance_id() { return next_tlas_instance_id++; }

  void upload_resources();

  void build_raytracing_pso(const std::wstring& shader_path,
    const d3d::RaytracingShaderData& shader_data, ComPtr<ID3D12StateObject>& pso);

  struct BuildAccelStruct {
    size_t instance_count;
    ID3D12Resource* const* blas;
    const UINT* instance_id;
    const Mat4* transform;
  };
  void build_dxr_acceleration_structure(
    BuildAccelStruct&& desc, ComPtr<ID3D12Resource>& tlas_buffer);

  void set_const_buffer(
    BufferHandle buffer, size_t index, size_t member, const void* value, size_t width);

  // TODO move to device resource
  D3D12_CPU_DESCRIPTOR_HANDLE get_rtv_cpu(ID3D12Resource* texture);
  D3D12_CPU_DESCRIPTOR_HANDLE get_dsv_cpu(ID3D12Resource* texture);

  // Handle createtions
  std::shared_ptr<d3d::BufferData> new_buffer() { return std::make_shared<d3d::BufferData>(this); }

  // Handle conversion

  std::shared_ptr<d3d::SwapchainData> to_swapchain(SwapchainHandle h) {
    return get_data<SwapchainHandle, d3d::SwapchainData>(h);
  }

  std::shared_ptr<d3d::BufferData> to_buffer(BufferHandle h) {
    return get_data<BufferHandle, d3d::BufferData>(h);
  }

  template <typename T>
  std::shared_ptr<T> to_shader(ShaderHandle h) {
    return get_data<ShaderHandle, T>(h);
  }

  std::shared_ptr<d3d::ShaderArgumentData> to_argument(ShaderArgumentHandle h) {
    return get_data<ShaderArgumentHandle, d3d::ShaderArgumentData>(h);
  }
};

} // namespace rei

#endif
