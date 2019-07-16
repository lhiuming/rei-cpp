#include "rt_path_tracing.h"

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
    space0.const_buffers = {ConstBuffer()};
    space0.shader_resources = {ShaderResource()};
    space0.unordered_accesses = {UnorderedAccess()};

    ShaderParameter space1 = {};
    //space1.const_buffers = {ConstBuffer()};
    space1.shader_resources = {ShaderResource(), ShaderResource()};

    global_signature.param_table = {space0};
    hitgroup_signature.param_table = {{}, space1};

    hitgroup_name = L"hit_group0";
    raygen_name = L"raygen_shader";
    //raygen_name = L"raygen_plain_color";
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
  // Model proxies
  // std::vector<MaterialHandle> dirty_materials;
  std::vector<ModelHandle> dirty_models;
};

} // namespace rtpt

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
  viewport->width = conf.width;
  viewport->height = conf.height;
  viewport->raytracing_output_buffer = renderer->create_unordered_access_buffer_2d(
    conf.width, conf.height, ResourceFormat::B8G8R8A8_UNORM);

  {
    ConstBufferLayout layout = {5};
    layout[0] = ShaderDataType::Float4x4; // proj to world
    layout[1] = ShaderDataType::Float4; // camera pos
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
  viewport->camera_pos = camera.position();
  viewport->camera_matrix_dirty = true;
}

RealtimePathTracingPipeline::SceneHandle RealtimePathTracingPipeline::register_scene(
  SceneConfig conf) {
  auto renderer = get_renderer();
  auto scene = make_shared<SceneData>();

  // We need Top-Level Acceleration Structure and shader table, about the whole scene
  scene->tlas_buffer = renderer->create_raytracing_accel_struct(*conf.scene);
  scene->shader_table = renderer->create_shader_table(*conf.scene, pathtracing_shader);

  // set up model properties in the scene
  for (auto& m : conf.scene->get_models()) {
    scene->dirty_models.push_back(m->get_rendering_handle());
  }

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
  REI_ASSERT(viewport);
  SceneData* scene = get_scene(scene_handle);
  REI_ASSERT(scene);
  Renderer* renderer = get_renderer();
  REI_ASSERT(renderer);

  renderer->begin_render();

  // TODO abstract a command list object; currently just an alias
  Renderer* cmd_list = renderer;

  // Update per-render buffer
  {
    BufferHandle pre_render_buffer = viewport->per_render_buffer;
    if (viewport->camera_matrix_dirty) {
      cmd_list->update_const_buffer(pre_render_buffer, 0, 0, viewport->view_proj_matrix.inv());
      cmd_list->update_const_buffer(pre_render_buffer, 0, 1, viewport->camera_pos);
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

  // Done
  renderer->present(viewport->swapchain, false);

  // Wait 
  // TODO remove this
  renderer->end_render();
}

} // namespace rei