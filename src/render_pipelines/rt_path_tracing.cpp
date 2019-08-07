#include "rt_path_tracing.h"

#include <vector>

#include "../direct3d/d3d_renderer.h"

using std::make_shared;
using std::make_unique;
using std::shared_ptr;
using std::unique_ptr;
using std::weak_ptr;

namespace rei {

struct PathTracingShaderMeta : RaytracingShaderMetaInfo {
  PathTracingShaderMeta() {
    ShaderParameter space0 = {};
    space0.const_buffers = {ConstantBuffer()};
    space0.shader_resources = {ShaderResource()};
    space0.unordered_accesses = {UnorderedAccess()};

    ShaderParameter space1 = {};
    // space1.const_buffers = {ConstBuffer()};
    space1.shader_resources = {ShaderResource(), ShaderResource()};

    global_signature.param_table = {space0};
    hitgroup_signature.param_table = {{}, space1};

    hitgroup_name = L"hit_group0";
    raygen_name = L"raygen_shader";
    // raygen_name = L"raygen_plain_color";
    closest_hit_name = L"closest_hit_shader";
    miss_name = L"miss_shader";
  }
};

namespace rtpt {
struct ViewportData {
  // Window viewport
  SwapchainHandle swapchain;
  size_t width, height;

  // Camera info
  bool camera_matrix_dirty = true;
  Mat4 view_matrix;
  Mat4 view_proj_matrix;
  Vec4 camera_pos;

  // Raytracing resources
  BufferHandle raytracing_output_buffer;
  BufferHandle per_render_buffer;
};

struct SceneData {
  // Raytracing resources
  BufferHandle tlas_buffer;
  BufferHandle shader_table;
  Hashmap<Scene::ModelUID, GeometryBufferHandles> geometry_buffers;
  // NOTE: index by instance id
  std::vector<ShaderArgumentHandle> shader_table_args;
};
} // namespace rtpt

using namespace rtpt;

RealtimePathTracingPipeline::RealtimePathTracingPipeline(RendererPtr renderer_wptr)
    : SimplexPipeline(std::dynamic_pointer_cast<d3d::Renderer>(renderer_wptr.lock())) {
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
  viewport->width = conf.width;
  viewport->height = conf.height;
  viewport->raytracing_output_buffer = renderer->create_unordered_access_buffer_2d(
    conf.width, conf.height, ResourceFormat::R8G8B8A8Unorm);

  {
    ConstBufferLayout layout = {5};
    layout[0] = ShaderDataType::Float4x4; // proj to world
    layout[1] = ShaderDataType::Float4;   // camera pos
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
  Renderer* r = get_renderer();
  REI_ASSERT(r);
  // Record camera transforms
  viewport->view_matrix = camera.world_to_camera();
  viewport->view_proj_matrix
    = r->is_depth_range_01() ? camera.world_to_device_halfz() : camera.world_to_device();
  viewport->camera_pos = camera.position();
  viewport->camera_matrix_dirty = true;
}

RealtimePathTracingPipeline::SceneHandle RealtimePathTracingPipeline::register_scene(
  SceneConfig conf) {
  auto renderer = get_renderer();
  auto scene = make_shared<SceneData>();

  // create mesh buffesr
  {
    for (auto& g : conf.scene->geometries()) {
      GeometryBufferHandles gb = renderer->create_geometry({g});
      scene->geometry_buffers.insert({conf.scene->get_id(g), gb});
    }
  }

  // create shader table arguments
  {
    ShaderArgumentValue arg_val {};
    for (auto& m : conf.scene->get_models()) {
      auto* geo = scene->geometry_buffers.try_get(conf.scene->get_id(m->get_geometry()));
      arg_val.shader_resources = {geo->index_buffer, geo->vertex_buffer};
      ShaderArgumentHandle h = renderer->create_shader_argument(arg_val);
      scene->shader_table_args.emplace_back(std::move(h));
    }
  }

  // We need Top-Level Acceleration Structure and shader table, about the whole scene
  {
    RaytraceSceneDesc desc;
    size_t inst_id_next = 0;
    for (auto& m : conf.scene->get_models()) {
      auto* geo = scene->geometry_buffers.try_get(conf.scene->get_id(m->get_geometry()));
      desc.blas_buffer.push_back(geo->blas_buffer);
      desc.transform.push_back(m->get_transform());
      desc.instance_id.push_back(inst_id_next++);
    }
    scene->tlas_buffer = renderer->create_raytracing_accel_struct(std::move(desc));
  }
  scene->shader_table = renderer->create_shader_table(*conf.scene, pathtracing_shader);

  auto handle = SceneHandle(scene.get());
  scenes.insert({handle, scene});
  return handle;
}

void RealtimePathTracingPipeline::render(ViewportHandle viewport_handle, SceneHandle scene_handle) {
  ViewportData* viewport = get_viewport(viewport_handle);
  REI_ASSERT(viewport);
  SceneData* scene = get_scene(scene_handle);
  REI_ASSERT(scene);
  Renderer* renderer = get_renderer();
  REI_ASSERT(renderer);

  // TODO abstract a command list object; currently just an alias
  Renderer* cmd_list = renderer->prepare();

  // Update per-render buffer
  {
    BufferHandle per_render_buffer = viewport->per_render_buffer;
    if (viewport->camera_matrix_dirty) {
      cmd_list->update_const_buffer(per_render_buffer, 0, 0, viewport->view_proj_matrix.inv());
      cmd_list->update_const_buffer(per_render_buffer, 0, 1, viewport->camera_pos);
      viewport->camera_matrix_dirty = false;
    }
  }

  // Update shader Tables
  {
    UpdateShaderTable update = UpdateShaderTable::hitgroup();
    update.shader = pathtracing_shader;
    update.shader_table = scene->shader_table;
    for (int index = 0; index < scene->shader_table_args.size(); index++) {
      update.index = index;
      update.arguments = {scene->shader_table_args[index]};
      cmd_list->update_shader_table(update);
    }
  }

  // Dispatch ray and write to raytracing output
  {
    // TODO cache argument by scene/viewport
    if (!pathtracing_argument) {
      ShaderArgumentValue args;
      args.const_buffers = {viewport->per_render_buffer};
      args.const_buffer_offsets = {0};
      args.shader_resources = {scene->tlas_buffer};
      args.unordered_accesses = {viewport->raytracing_output_buffer};
      pathtracing_argument = renderer->create_shader_argument(args);
    }

    cmd_list->raytrace(pathtracing_shader, {pathtracing_argument}, scene->shader_table,
      viewport->width, viewport->height);
  }

  // Copy to render target
  BufferHandle rt_buffer = renderer->fetch_swapchain_render_target_buffer(viewport->swapchain);
  { cmd_list->copy_texture(viewport->raytracing_output_buffer, rt_buffer, true); }

  // Done
  cmd_list->transition(rt_buffer, ResourceState::Present);
  renderer->present(viewport->swapchain, false);
}

} // namespace rei