#include "hybrid.h"

#include <memory>
#include <unordered_map>

#include "../container_utils.h"
#include "../direct3d/d3d_renderer.h"

namespace rei {

namespace hybrid {

struct ViewportProxy {
  size_t width = 1;
  size_t height = 1;
  SwapchainHandle swapchain;
  BufferHandle normal_buffer;
  BufferHandle albedo_buffer;

  BufferHandle raytracing_output_buffer;

  ShaderArgumentHandle blit_after_rt_argument;

  Vec4 cam_pos = {0, 1, 8, 1};
  Mat4 view_proj = Mat4::I();
  Mat4 view_proj_inv = Mat4::I();
};

struct SceneProxy {
  // holding for default material
  BufferHandle objects_cb;
  BufferHandle materials_cb;
  struct MaterialData {
    ShaderArgumentHandle arg;
    Vec4 albedo;
    Vec4 smoothness_metalness_zw;
    size_t cb_index;
  };
  struct ModelData {
    GeometryHandle geometry;
    // TODO support root cb offseting
    ShaderArgumentHandle arg;
    size_t cb_index;
    Mat4 trans;

    // JUST COPY THAT
    MaterialData mat;
  };
  Hashmap<MaterialPtr, MaterialData> materials;
  Hashmap<ModelHandle, ModelData> models;

  // Accelration structure and shader table
  BufferHandle tlas;
  BufferHandle shader_table;
};

} // namespace hybrid

using namespace hybrid;

struct HybridGPassShaderDesc : RasterizationShaderMetaInfo {
  HybridGPassShaderDesc() {
    ShaderParameter space0 {};
    space0.const_buffers = {ConstBuffer()};
    ShaderParameter space1 {};
    space1.const_buffers = {ConstBuffer()};
    ShaderParameter space2 {};
    space2.const_buffers = {ConstBuffer()};

    RenderTargetDesc rt_normal {ResourceFormat::R32G32B32A32_FLOAT};
    RenderTargetDesc rt_albedo {ResourceFormat::B8G8R8A8_UNORM};
    render_target_descs = {rt_normal, rt_albedo};

    signature.param_table = {space0, space1};
  }
};

struct HybridRaytracingShaderDesc : RaytracingShaderMetaInfo {
  HybridRaytracingShaderDesc() {
    ShaderParameter space0 = {};
    space0.const_buffers = {ConstBuffer()}; // Per-render CN
    space0.shader_resources = {
      ShaderResource(), ShaderResource(), ShaderResource(), ShaderResource()}; // TLAS ans G-Buffer
    space0.unordered_accesses = {UnorderedAccess()};                           // output buffer
    global_signature.param_table = {space0};

    ShaderParameter space1 = {};
    space1.shader_resources = {ShaderResource(), ShaderResource()}; // Mesh indices and vertices
    hitgroup_signature.param_table = {{}, space1};

    hitgroup_name = L"hit_group0";
    raygen_name = L"raygen_shader";
    // raygen_name = L"raygen_plain_color";
    closest_hit_name = L"closest_hit_shader";
    miss_name = L"miss_shader";
  }
};

struct BlitShaderDesc : RasterizationShaderMetaInfo {
  BlitShaderDesc() {
    ShaderParameter space0 {};
    space0.shader_resources = {ShaderResource()};
    space0.static_samplers = {StaticSampler()};
    signature.param_table = {space0};

    this->is_depth_stencil_disabled = true;

    RenderTargetDesc dest {ResourceFormat::B8G8R8A8_UNORM};
    this->render_target_descs = {dest};
  }
};

HybridPipeline::HybridPipeline(RendererPtr renderer) : SimplexPipeline(renderer) {
  Renderer* r = get_renderer();

  {
    m_gpass_shader
      = r->create_shader(L"CoreData/shader/hybrid_gpass.hlsl", HybridGPassShaderDesc());
  }

  {
    m_raytraced_lighting_shader
      = r->create_shader(L"CoreData/shader/hybrid_raytracing.hlsl", HybridRaytracingShaderDesc());
  }

  { m_blit_shader = r->create_shader(L"CoreData/shader/blit.hlsl", BlitShaderDesc()); }

  {
    ConstBufferLayout lo = {
      ShaderDataType::Float4,   // screen size
      ShaderDataType::Float4x4, // proj_to_world := camera view_proj inverse
      ShaderDataType::Float4,   // camera pos
    };
    m_per_render_buffer = r->create_const_buffer(lo, 1, L"PerRenderCB");
  }
}

HybridPipeline::ViewportHandle HybridPipeline::register_viewport(ViewportConfig conf) {
  Renderer* r = get_renderer();

  ViewportProxy proxy {};

  proxy.width = conf.width;
  proxy.height = conf.height;
  proxy.swapchain = r->create_swapchain(conf.window_id, conf.width, conf.height, 2);

  proxy.normal_buffer = r->create_texture_2d(
    conf.width, conf.height, ResourceFormat::R32G32B32A32_FLOAT, L"Normal Buffer");
  proxy.albedo_buffer = r->create_texture_2d(
    conf.width, conf.height, ResourceFormat::B8G8R8A8_UNORM, L"Albedo Buffer");

  proxy.raytracing_output_buffer = r->create_unordered_access_buffer_2d(
    conf.width, conf.height, ResourceFormat::R32G32B32A32_FLOAT, L"Raytracing Output Buffer");

  {
    ShaderArgumentValue val {};
    val.shader_resources = {proxy.raytracing_output_buffer};
    proxy.blit_after_rt_argument = r->create_shader_argument(val);
  }

  return add_viewport(std::move(proxy));
}

void HybridPipeline::transform_viewport(ViewportHandle handle, const Camera& camera) {
  ViewportProxy* viewport = get_viewport(handle);
  REI_ASSERT(viewport);
  Renderer* r = get_renderer();
  REI_ASSERT(r);

  viewport->cam_pos = camera.position();
  viewport->view_proj = r->is_depth_range_01() ? camera.view_proj_halfz() : camera.view_proj();
  viewport->view_proj_inv = viewport->view_proj.inv();
}

HybridPipeline::SceneHandle HybridPipeline::register_scene(SceneConfig conf) {
  const Scene* scene = conf.scene;
  REI_ASSERT(scene);
  Renderer* r = get_renderer();
  REI_ASSERT(r);

  const size_t model_count = scene->get_models().size();
  const size_t material_count = scene->materials().size();

  SceneProxy proxy = {};
  // Initialize Models
  {
    ConstBufferLayout mat_lo = {
      ShaderDataType::Float4, // albedo
      ShaderDataType::Float4, // metalness and smoothness
    };
    proxy.materials_cb = r->create_const_buffer(mat_lo, material_count, L"Scene Material CB");

    proxy.materials.reserve(material_count);
    size_t index = 0;
    ShaderArgumentValue v {};
    v.const_buffers = {proxy.materials_cb}; // currenlty, materials are just somt value config
    v.const_buffer_offsets = {0};
    for (const MaterialPtr& mat : scene->materials()) {
      v.const_buffer_offsets[0] = index;
      ShaderArgumentHandle mat_arg = r->create_shader_argument(v);
      REI_ASSERT(mat_arg);
      SceneProxy::MaterialData data {};
      data.arg = mat_arg;
      data.cb_index = index;
      data.albedo = Vec4(mat->get<Color>(L"albedo").value_or(Colors::magenta));
      data.smoothness_metalness_zw.x = mat->get<double>(L"smoothness").value_or(0);
      data.smoothness_metalness_zw.y = mat->get<double>(L"metalness").value_or(0);
      proxy.materials.insert({mat, data});
      index++;
    }
  }

  // Initialize Geometry, Material, and assign per-object c-buffer
  {
    ConstBufferLayout cb_lo = {
      ShaderDataType::Float4x4, // WVP
      ShaderDataType::Float4x4, // World
    };
    proxy.objects_cb = r->create_const_buffer(cb_lo, model_count, L"Scene-Objects CB");

    proxy.models.reserve(model_count);
    ShaderArgumentValue arg_value = {};
    { // init
      arg_value.const_buffers = {proxy.objects_cb};
      arg_value.const_buffer_offsets = {0};
    }
    for (size_t i = 0; i < model_count; i++) {
      auto model = scene->get_models()[i];
      arg_value.const_buffer_offsets[0] = i;
      ShaderArgumentHandle arg = r->create_shader_argument(arg_value);
      auto mat = proxy.materials.try_get(model->get_material());
      REI_WARNINGIF(mat == nullptr);
      proxy.models.insert({
        model->get_rendering_handle(),                                                      // key
        {model->get_geometry()->get_graphic_handle(), arg, i, model->get_transform(), *mat} // value
      });
    }
  }

  // Build Acceleration structure and shader table
  {
    proxy.tlas = r->create_raytracing_accel_struct(*scene);
    proxy.shader_table = r->create_shader_table(*scene, m_raytraced_lighting_shader);
  }

  return add_scene(std::move(proxy));
}

void HybridPipeline::update_model(SceneHandle scene_handle, const Model& model) {
  SceneProxy* scene = get_scene(scene_handle);
  REI_ASSERT(scene);
  auto* data = scene->models.try_get(model.get_rendering_handle());
  data->geometry = model.get_geometry()->get_graphic_handle();
  data->trans = model.get_transform();
}

void HybridPipeline::render(ViewportHandle viewport_h, SceneHandle scene_h) {
  ViewportProxy* viewport = get_viewport(viewport_h);
  SceneProxy* scene = get_scene(scene_h);
  REI_ASSERT(viewport);
  REI_ASSERT(scene);

  Renderer* const renderer = get_renderer();
  const auto cmd_list = renderer->prepare();

  // Update scene-wide const buffer (per-frame CB)
  {
    Vec4 screen = Vec4(viewport->width, viewport->height, 0, 0);
    cmd_list->update_const_buffer(m_per_render_buffer, 0, 0, screen);
    cmd_list->update_const_buffer(m_per_render_buffer, 0, 1, viewport->view_proj_inv);
    cmd_list->update_const_buffer(m_per_render_buffer, 0, 2, viewport->cam_pos);
  }

  // Update material buffer
  {
    for (auto& pair : scene->materials) {
      auto& mat = pair.second;
      renderer->update_const_buffer(scene->materials_cb, mat.cb_index, 0, mat.albedo);
      renderer->update_const_buffer(scene->materials_cb, mat.cb_index, 1, mat.smoothness_metalness_zw);
    }
  }

  // Update per-object const buffer
  {
    for (auto& pair : scene->models) {
      auto& model = pair.second;
      size_t index = model.cb_index;
      Mat4& m = model.trans;
      Mat4 mvp = viewport->view_proj * m;
      renderer->update_const_buffer(scene->objects_cb, index, 0, mvp);
      renderer->update_const_buffer(scene->objects_cb, index, 1, m);
    }
  }

  // Geometry pass
  BufferHandle ds_buffer = renderer->fetch_swapchain_depth_stencil_buffer(viewport->swapchain);
  cmd_list->transition(viewport->normal_buffer, ResourceState::RenderTarget);
  cmd_list->transition(viewport->albedo_buffer, ResourceState::RenderTarget);
  cmd_list->transition(ds_buffer, ResourceState::DeptpWrite);
  RenderPassCommand gpass = {};
  {
    gpass.render_targets = {viewport->normal_buffer, viewport->albedo_buffer};
    gpass.depth_stencil = ds_buffer;
    gpass.clear_ds = true;
    gpass.clear_rt = true;
    gpass.area = {viewport->width, viewport->height};
  }
  cmd_list->begin_render_pass(gpass);
  {
    DrawCommand draw_cmd = {};
    draw_cmd.shader = m_gpass_shader;
    for (auto pair : scene->models) {
      auto& model = pair.second;
      draw_cmd.geo = model.geometry;
      draw_cmd.arguments = {model.arg, model.mat.arg};
      cmd_list->draw(draw_cmd);
    }
  }
  cmd_list->end_render_pass();

  // Update shader Tables
  {
    for (auto& pair : scene->models) {
      ModelHandle h = pair.first;
      cmd_list->update_hitgroup_shader_record(scene->shader_table, h);
    }
  }

  // Raytraced lighting
  {
    RaytraceCommand cmd {};
    cmd.raytrace_shader = m_raytraced_lighting_shader;
    cmd.arguments = {fetch_raytracing_arg(viewport_h, scene_h)};
    cmd.shader_table = scene->shader_table;
    cmd.width = viewport->width;
    cmd.height = viewport->height;
    cmd_list->raytrace(cmd);
  }

  // Blit raytracing result for debug
  BufferHandle render_target = renderer->fetch_swapchain_render_target_buffer(viewport->swapchain);
  cmd_list->transition(render_target, ResourceState::RenderTarget);
  cmd_list->transition(viewport->raytracing_output_buffer, ResourceState::PixelShaderResource);
  {
    RenderPassCommand render_pass {};
    render_pass.render_targets = {render_target};
    render_pass.depth_stencil = c_empty_handle;
    render_pass.clear_ds = false;
    render_pass.clear_rt = true;
    render_pass.area = {viewport->width, viewport->height};
    cmd_list->begin_render_pass(render_pass);

    DrawCommand draw {};
    draw.arguments = {viewport->blit_after_rt_argument};
    draw.geo = c_empty_handle;
    draw.shader = m_blit_shader;
    cmd_list->draw(draw);

    cmd_list->end_render_pass();
  }

  // Shading pass
  if (false) {
    cmd_list->transition(render_target, ResourceState::RenderTarget);
    cmd_list->transition(viewport->normal_buffer, ResourceState::PixelShaderResource);
    cmd_list->transition(viewport->albedo_buffer, ResourceState::PixelShaderResource);
    cmd_list->transition(ds_buffer, ResourceState::PixelShaderResource);
    RenderPassCommand shading_pass = {};
    {
      shading_pass.render_targets = {render_target};
      shading_pass.depth_stencil = c_empty_handle;
      shading_pass.clear_rt = true;
      shading_pass.clear_ds = false;
      shading_pass.area = {viewport->width, viewport->height};
    }
    cmd_list->begin_render_pass(shading_pass);
    DrawCommand cmd = {};
    cmd.geo = c_empty_handle;
    cmd.shader = c_empty_handle;
    cmd.arguments = {};
    cmd_list->draw(cmd);
    cmd_list->end_render_pass();
  }

  cmd_list->transition(render_target, ResourceState::Present);
  cmd_list->present(viewport->swapchain, false);
}

ShaderArgumentHandle HybridPipeline::fetch_raytracing_arg(
  ViewportHandle viewport_h, SceneHandle scene_h) {
  const RaytracingArgumentKey cache_key {viewport_h, scene_h};

  // check cache
  ShaderArgumentHandle* cached_arg = m_raytracing_args.try_get(cache_key);
  if (cached_arg) { return *cached_arg; }

  // create new one
  Renderer* r = get_renderer();
  REI_ASSERT(r);
  ViewportProxy* viewport = get_viewport(viewport_h);
  SceneProxy* scene = get_scene(scene_h);
  REI_ASSERT(viewport && scene);

  ShaderArgumentValue val {};
  val.const_buffers = {m_per_render_buffer};
  val.const_buffer_offsets = {0};
  val.shader_resources = {
    scene->tlas,                                                  // t0 TLAS
    r->fetch_swapchain_depth_stencil_buffer(viewport->swapchain), // t1 G-buffer depth
    viewport->normal_buffer,                                      // t2 G-Buffer::normal
    viewport->albedo_buffer,                                      // t3 G-Buffer::albedo
  };
  val.unordered_accesses = {viewport->raytracing_output_buffer};
  ShaderArgumentHandle arg = r->create_shader_argument(val);
  REI_ASSERT(arg);

  // add to cache
  m_raytracing_args.insert({cache_key, arg});

  return arg;
}

} // namespace rei
