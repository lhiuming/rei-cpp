#ifndef REI_DIRECT3D_D3D_RENDERER_H
#define REI_DIRECT3D_D3D_RENDERER_H

#if DIRECT3D_ENABLED

#include <array>
#include <memory>

#include "../algebra.h"
#include "../camera.h"
#include "../common.h"
#include "../model.h"
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

enum RenderMode {
  Rasterization,
  RealtimeRaytracing,
};

class Renderer : public rei::Renderer {
  using Self = typename Renderer;
  using Base = typename rei::Renderer;

public:
  struct Options {
    RenderMode init_render_mode = RenderMode::Rasterization;
    bool enable_realtime_raytracing = false;
    // debug switched
    bool draw_debug_model = false;
  };

  Renderer(HINSTANCE hinstance, Options options = {});
  ~Renderer() override;

  SwapchainHandle create_swapchain(SystemWindowID window_id, size_t width, size_t height, size_t rendertarget_count);
  BufferHandle fetch_swapchain_depth_stencil_buffer(SwapchainHandle swapchain);
  BufferHandle fetch_swapchain_render_target_buffer(SwapchainHandle swapchain);

  ScreenTransformHandle create_viewport(int width, int height);
  void set_viewport_clear_value(ScreenTransformHandle viewport, Color color) override;
  void update_viewport_vsync(ScreenTransformHandle viewport, bool enabled_vsync) override;
  void update_viewport_size(ScreenTransformHandle viewport, int width, int height) override;
  void update_viewport_transform(ScreenTransformHandle viewport, const Camera& camera) override;

  BufferHandle create_unordered_access_buffer_2d(
    size_t width, size_t height, ResourceFormat format);
  BufferHandle create_const_buffer(const ConstBufferLayout& layout, size_t num);

  void update_const_buffer(BufferHandle buffer, size_t index, size_t member, Vec4 value);
  void update_const_buffer(BufferHandle buffer, size_t index, size_t member, Mat4 value);

  ShaderHandle create_shader(
    const std::wstring& shader_path, std::unique_ptr<RasterizationShaderMetaInfo>&& meta);
  ShaderHandle create_raytracing_shader(
    const std::wstring& shader_path, std::unique_ptr<::rei::RaytracingShaderMetaInfo>&& meta);

  ShaderArgumentHandle create_shader_argument(ShaderHandle shader, ShaderArgumentValue arg_value);
  void update_shader_argument(ShaderHandle shader, ShaderArgumentValue arg_value, ShaderArgumentHandle& arg_handle);

  GeometryHandle create_geometry(const Geometry& geometry) override;
  ModelHandle create_model(const Model& model) override;

  BufferHandle create_raytracing_accel_struct(const Scene& scene);
  BufferHandle create_shader_table(const Scene& scene, ShaderHandle raytracing_shader);

  //void update_raygen_shader_record();
  void update_hitgroup_shader_record(BufferHandle shader_table, ModelHandle model);

  void begin_render();
  void end_render();

  using ShaderArguments = v_array<ShaderArgumentHandle, 8>;
  void raytrace(ShaderHandle raytrace_shader, ShaderArguments arguments, BufferHandle shader_table, size_t width, size_t height, size_t depth = 1);
  void copy_texture(BufferHandle src, BufferHandle dest, bool revert_state = true);

  void present(SwapchainHandle handle, bool vsync);

  DeviceResources& device() const { return *device_resources; }

  void prepare(Scene& scene) override;
  CullingResult cull(ScreenTransformHandle viewport, const Scene& scene) override;

protected:
  HINSTANCE hinstance;
  RenderMode mode;
  const bool is_dxr_enabled;
  const bool draw_debug_model;

  // some const config
  constexpr static VectorTarget model_trans_target = VectorTarget::Column;

  // TODO make unique
  std::shared_ptr<DeviceResources> device_resources;
  // TODO deprecate this
  std::vector<std::shared_ptr<ViewportResources>> viewport_resources_lib;

  std::shared_ptr<MeshData> default_geometry;
  std::shared_ptr<ShaderData> default_shader;
  std::shared_ptr<MaterialData> default_material;
  std::shared_ptr<ModelData> debug_model;

  bool is_uploading_resources = false;

  // Deferred resources

  ShaderHandle deferred_base_pass;
  ShaderHandle deferred_shading_pass;

  // Ray Tracing Resources //
  UINT next_tlas_instance_id = 0;
  UINT generate_tlas_instance_id() { return next_tlas_instance_id++; }

  // std::unique_ptr<UploadBuffer<dxr::PerFrameConstantBuffer>> m_perframe_cb;

  ComPtr<ID3D12Resource> raygen_shader_table;
  ComPtr<ID3D12Resource> miss_shader_table;

  // void* m_hitgroup_shader_id;
  // std::unique_ptr<ShaderTable<dxr::HitgroupRootArguments>> m_hitgroup_shader_table;

  void upload_resources();
  void render(ViewportData& viewport, CullingData& culling);

  struct ModelDrawTask {
    std::size_t model_num;
    const ModelData* models;
    const Mat4* view_proj;
    const RenderTargetSpec* target_spec;
  };

  void draw_meshes(ID3D12GraphicsCommandList& cmd_list, ModelDrawTask& task,
    ShaderData* shader_override = nullptr);

  void build_raytracing_pso(const std::wstring& shader_path,
    const d3d::RaytracingShaderData& shader_data, ComPtr<ID3D12StateObject>& pso);
  void build_dxr_acceleration_structure(ModelData* models, std::size_t m_count,
    ComPtr<ID3D12Resource>& scratch_buffer, ComPtr<ID3D12Resource>& tlas_buffer);

  void set_const_buffer(
    BufferHandle buffer, size_t index, size_t member, const void* value, size_t width);

  // Debug support
  void create_default_assets();

  // Handle conversion

  std::shared_ptr<ViewportData> to_viewport(ScreenTransformHandle h) {
    return get_data<ScreenTransformHandle, ViewportData>(h);
  }

  std::shared_ptr<SwapchainData> to_swapchain(SwapchainHandle h) {
    return get_data<SwapchainHandle, SwapchainData>(h);
  }

  std::shared_ptr<CullingData> to_culling(CullingResult h) {
    return get_data<CullingResult, CullingData>(h);
  }
  std::shared_ptr<ModelData> to_model(ModelHandle h) { return get_data<ModelHandle, ModelData>(h); }
  template <typename BufferType>
  std::shared_ptr<BufferType> to_buffer(BufferHandle h) {
    return get_data<BufferHandle, BufferType>(h);
  }

  template <typename T>
  std::shared_ptr<T> to_shader(ShaderHandle h) {
    return get_data<ShaderHandle, T>(h);
  }

  std::shared_ptr<ShaderArgumentData> to_argument(ShaderArgumentHandle h) {
    return get_data<ShaderArgumentHandle, ShaderArgumentData>(h);
  }

  template <typename DataType>
  std::shared_ptr<DataType> to_geometry(GeometryHandle h) {
    return get_data<GeometryHandle, DataType>(h);
  }
  std::shared_ptr<GeometryData> to_geometry(GeometryHandle h) {
    return get_data<GeometryHandle, GeometryData>(h);
  }
  std::shared_ptr<MaterialData> to_material(MaterialHandle h) {
    return get_data<MaterialHandle, MaterialData>(h);
  }
};

} // namespace d3d

} // namespace rei

#endif

#endif
