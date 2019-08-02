#ifndef REI_DIRECT3D_D3D_RENDERER_H
#define REI_DIRECT3D_D3D_RENDERER_H

#if DIRECT3D_ENABLED

#include <array>
#include <memory>

#include "../algebra.h"
#include "../camera.h"
#include "../common.h"
#include "../renderer.h"
#include "../scene.h"

/*
 * direct3d/d3d_renderer.h
 * Implement a Direct3D11-based renderer.
 */
#include "d3d_common_resources.h"

namespace rei {

namespace d3d {

class DeviceResources;
class ViewportResources;
struct ShaderData;
struct ViewportData;
struct CullingData;

class Renderer : public rei::Renderer {
  using Self = typename Renderer;
  using Base = typename rei::Renderer;

public:
  struct Options {
    bool enable_realtime_raytracing = true;
  };

  Renderer(HINSTANCE hinstance, Options options = {});
  ~Renderer() override = default;

  SwapchainHandle create_swapchain(
    SystemWindowID window_id, size_t width, size_t height, size_t rendertarget_count);
  BufferHandle fetch_swapchain_render_target_buffer(SwapchainHandle swapchain);

  BufferHandle create_texture_2d(
    const TextureDesc& desc, ResourceState init_state, std::wstring&& debug_name);
  [[deprecated]] BufferHandle create_texture_2d(
    size_t width, size_t height, ResourceFormat format, std::wstring&& debug_name) {
    TextureDesc desc {width, height, format};
    return create_texture_2d(desc, ResourceState::Undefined, std::move(debug_name));
  }
  [[deprecated]] BufferHandle create_unordered_access_buffer_2d(size_t width, size_t height,
    ResourceFormat format, std::wstring&& debug_name = L"Unnamed UA Buffer") {
    return create_texture_2d(TextureDesc::unorder_access(width, height, format),
      ResourceState::UnorderedAccess, std ::move(debug_name));
  }

  BufferHandle create_const_buffer(const ConstBufferLayout& layout, size_t num,
    std::wstring&& debug_name = L"Unnamed ConstBuffer");

  void update_const_buffer(BufferHandle buffer, size_t index, size_t member, Vec4 value);
  void update_const_buffer(BufferHandle buffer, size_t index, size_t member, Mat4 value);

  ShaderHandle create_shader(const std::wstring& shader_path, RasterizationShaderMetaInfo&& meta,
    const ShaderCompileConfig& config = {});
  ShaderHandle create_shader(const std::wstring& shader_path,
    std::unique_ptr<RasterizationShaderMetaInfo>&& meta, const ShaderCompileConfig& config = {}) {
    return create_shader(shader_path, std::move(*meta), config);
  }

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

  GeometryBuffers create_geometry(const GeometryDesc& geometry);
  BufferHandle create_raytracing_accel_struct(const RaytraceSceneDesc& scene);
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

  bool is_depth_range_01() const override { return true; }

protected:
  HINSTANCE hinstance;
  const bool is_dxr_enabled;

  // some const config
  constexpr static VectorTarget model_trans_target = VectorTarget::Column;

  // TODO make unique
  std::shared_ptr<DeviceResources> device_resources;
  std::vector<ComPtr<ID3D12Resource>> m_delayed_release;
  Hashmap<BufferHandle, UINT> m_texture_clear_descriptor;

  bool is_uploading_resources = false;

  Hashmap<const ID3D12Resource*, D3D12_CPU_DESCRIPTOR_HANDLE> m_dsv_cache {};
  Hashmap<const ID3D12Resource*, D3D12_CPU_DESCRIPTOR_HANDLE> m_rtv_cache {};

  // TODO move state to cmd_list object
  const RenderTargetSpec curr_target_spec {};

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
  void build_dxr_acceleration_structure(BuildAccelStruct&& desc,
    ComPtr<ID3D12Resource>& scratch_buffer, ComPtr<ID3D12Resource>& tlas_buffer);

  void set_const_buffer(
    BufferHandle buffer, size_t index, size_t member, const void* value, size_t width);

  // TODO move to device resource
  D3D12_CPU_DESCRIPTOR_HANDLE get_rtv_cpu(ID3D12Resource* texture);
  D3D12_CPU_DESCRIPTOR_HANDLE get_dsv_cpu(ID3D12Resource* texture);

  // Handle createtions
  std::shared_ptr<BufferData> new_buffer() { return std::make_shared<BufferData>(this); }

  // Handle conversion

  std::shared_ptr<SwapchainData> to_swapchain(SwapchainHandle h) {
    return get_data<SwapchainHandle, SwapchainData>(h);
  }

  std::shared_ptr<BufferData> to_buffer(BufferHandle h) {
    return get_data<BufferHandle, BufferData>(h);
  }

  template <typename T>
  std::shared_ptr<T> to_shader(ShaderHandle h) {
    return get_data<ShaderHandle, T>(h);
  }

  std::shared_ptr<ShaderArgumentData> to_argument(ShaderArgumentHandle h) {
    return get_data<ShaderArgumentHandle, ShaderArgumentData>(h);
  }
};

} // namespace d3d

} // namespace rei

#endif

#endif
