#include "rt_path_tracing.h"

using std::make_shared;
using std::make_unique;
using std::shared_ptr;
using std::unique_ptr;
using std::weak_ptr;

namespace rei {

/*
 * Hard code the root siagnature for dxr kick up
 */
struct GlobalSignatureLayout {
  enum CBV { PreFrameCBV = 0, CBV_Count };
  enum SRV { AccelStruct = 0, SRV_COUNT };
  enum UAV { OutputTex = 0, UAV_COUNT };
};

struct RaygenRSLayout {};

struct HitgroupRSLayout {
  enum CBV { PerObjectCBV = 0, CBV_Count };
  enum SRV { MeshIndexSRV = 0, MeshVertexSRV, SRV_Count };
};

/*
struct PerObjectConstantBuffer {
};

struct HitgroupRootArguments {
  D3D12_GPU_DESCRIPTOR_HANDLE mesh_buffer_table_start;
};
*/

// FIXME remove this hardcode
constexpr size_t c_hitgroup_size = 32;

/*
struct PerFrameConstantBuffer {
  DirectX::XMMATRIX proj_to_world;
  DirectX::XMFLOAT4 camera_pos;
  DirectX::XMFLOAT4 ambient_color;
  DirectX::XMFLOAT4 light_pos;
  DirectX::XMFLOAT4 light_color;
};
*/

constexpr wchar_t* c_hit_group_name = L"hit_group0";
constexpr wchar_t* c_raygen_shader_name = L"raygen_shader";
constexpr wchar_t* c_closest_hit_shader_name = L"closest_hit_shader";
constexpr wchar_t* c_miss_shader_name = L"miss_shader";

struct PathTracingShaderMeta : RaytracingShaderMetaInfo {
  PathTracingShaderMeta() {
    ShaderParameter space0 = {};
    space0.const_buffers = {ConstBuffer()};
    space0.shader_resources = {ShaderResource()};
    space0.unordered_accesses = {UnorderedAccess()};

    ShaderParameter space1 = {};
    space1.const_buffers = {ConstBuffer()};
    space1.shader_resources = {ShaderResource()};

    global_signature.param_table = {space0};
    hitgroup_signature.param_table = {{}, space1};
  }
};

RealtimePathTracingPipeline::RealtimePathTracingPipeline(weak_ptr<rei::Renderer> renderer_wptr)
    : m_renderer(std::dynamic_pointer_cast<d3d::Renderer>(renderer_wptr.lock())) {
  auto renderer = m_renderer.lock();
  std::wstring shader_path = L"CoreData/shader/raytracing.hlsl";
  pathtracing_shader
    = renderer->create_raytracing_shader(shader_path, make_unique<PathTracingShaderMeta>());
}

RealtimePathTracingPipeline::ViewportHandle RealtimePathTracingPipeline::register_viewport(
  ViewportConfig conf) {
  auto renderer = get_renderer();
  auto viewport = make_shared<ViewportData>();

  viewport->swapchain = renderer->create_swapchain(conf.window_id, conf.width, conf.height, 2);
  viewport->view_transform = renderer->create_viewport(conf.width, conf.height);
  viewport->width = conf.width;
  viewport->height = conf.height;
  viewport->raytracing_output_buffer = renderer->create_unordered_access_buffer_2d(
    conf.width, conf.height, ResourceFormat::R32G32B32A32_FLOAT);

  {
    /*
struct PerFrameConstantBuffer {
  DirectX::XMMATRIX proj_to_world;
  DirectX::XMFLOAT4 camera_pos;
  DirectX::XMFLOAT4 ambient_color;
  DirectX::XMFLOAT4 light_pos;
  DirectX::XMFLOAT4 light_color;
};
     */

    ConstBufferLayout layout = {5};
    layout[0] = ShaderDataType::Float4x4;
    layout[1] = ShaderDataType::Float4;
    layout[2] = ShaderDataType::Float4;
    layout[3] = ShaderDataType::Float4;
    layout[4] = ShaderDataType::Float4;
    viewport->per_render_buffer = renderer->create_const_buffer(layout, 1);
  }

  auto handle = ViewportHandle(viewport.get());
  viewports.insert({handle, viewport});
  return handle;
}

void RealtimePathTracingPipeline::transform_viewport(ViewportHandle handle, const Camera& camera) {
  ViewportData* viewport = get_viewport(handle);
  REI_ASSERT(viewport);
  // Record camera transforms
  viewport->view_matrix = camera.world_to_camera();
  viewport->view_proj_matrix = camera.world_to_device_halfz(); // TODO check if using direct3D
  viewport->camera_matrix_dirty = true;
}

RealtimePathTracingPipeline::SceneHandle RealtimePathTracingPipeline::register_scene(
  SceneConfig conf) {
  auto renderer = get_renderer();
  auto scene = make_shared<SceneData>();

  // We need Top-Level Acceleration Structure and shader table, about the whole scene
  scene->tlas_buffer = renderer->create_raytracing_accel_struct(*conf.scene);
  scene->shader_table = renderer->create_shder_table(*conf.scene, pathtracing_shader);

  auto handle = SceneHandle(scene.get());
  scenes.insert({handle, scene});
  return handle;
}

void RealtimePathTracingPipeline::add_model(SceneHandle scene_h, ModelHandle model) {
  SceneData* scene = get_scene(scene_h);
  REI_ASSERT(scene);
  auto renderer = get_renderer();
  scene->dirty_models.push_back(model);
}

void RealtimePathTracingPipeline::render(ViewportHandle viewport_handle, SceneHandle scene_handle) {
  ViewportData* viewport = get_viewport(viewport_handle);
  SceneData* scene = get_scene(scene_handle);
  Renderer* renderer = get_renderer();

  renderer->begin_render();

  // TODO abstract a command list object; currently just an alias
  Renderer* cmd_list = renderer;

  // Update per-render buffer
  {
    BufferHandle pre_render_buffer = viewport->per_render_buffer;
    if (viewport->camera_matrix_dirty) {
      cmd_list->update_const_buffer(pre_render_buffer, 0, 0, viewport->view_proj_matrix.inv());
      viewport->camera_matrix_dirty = false;
    }
  }

  // Update shader Tables
  {
    for (ModelHandle model : scene->dirty_models) {
      cmd_list->update_hitgroup_shader_record(scene->shader_table, model);
    }
    scene->dirty_models.clear();
  }

  // Dispatch ray and write to raytracing output
  {
    // TODO cache argument by scene/viewport
    if (!pathtracing_argument) {
      ShaderArgumentValue args;
      args.const_buffers = {viewport->per_render_buffer};
      args.shader_resources = {scene->tlas_buffer};
      args.unordered_accesses = {viewport->raytracing_output_buffer};

      pathtracing_argument = renderer->create_shader_argument(pathtracing_shader, args);
    }

    cmd_list->raytrace(pathtracing_shader, {pathtracing_argument}, scene->shader_table,
      viewport->width, viewport->height);
  }

  // Copy to render target
  {
    BufferHandle rt_buffer = renderer->fetch_swapchain_render_target_buffer(viewport->swapchain);
    cmd_list->copy_texture(viewport->raytracing_output_buffer, rt_buffer);
  }

  // Done and Present
  renderer->present(viewport->swapchain, true);

  // Wait
  cmd_list->close_cmd_list();
  renderer->end_render();
}

} // namespace rei