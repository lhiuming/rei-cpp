#include "deferred.h"

#include <memory>
#include <unordered_map>

#include "../container_utils.h"
#include "../renderer.h"

namespace rei {

namespace deferred {

struct ViewportData {
  size_t width = 1;
  size_t height = 1;
  SwapchainHandle swapchain;
  BufferHandle depth_stencil_buffer;
  BufferHandle normal_buffer;
  BufferHandle albedo_buffer;
  ShaderArgumentHandle shading_pass_per_render_arg;
  // Max 8 light pass
  FixedVec<ShaderArgumentHandle, DeferredPipeline::max_light_count> shading_pass_per_light_args;

  Mat4 view_proj = Mat4::I();
  Mat4 view_proj_inv = Mat4::I();
  Vec4 cam_pos = {0, 1, 8, 1};
};

struct SceneData {
  // holding for default material
  BufferHandle objects_cb;
  struct ModelData {
    GeometryBufferHandles geometry;
    // TODO should use an offset in cb instead, rather than a brand-new descriptor
    ShaderArgumentHandle arg;
    size_t cb_index;
    Mat4 trans;
  };
  Hashmap<Scene::GeometryUID, GeometryBufferHandles> geometries;
  Hashmap<Scene::ModelUID, ModelData> m_models;
};

} // namespace deferred

using ViewportProxy = deferred::ViewportData;
using SceneProxy = deferred::SceneData;

struct DeferredBaseMeta : RasterizationShaderMetaInfo {
  DeferredBaseMeta() {
    ShaderParameter space0 {};
    space0.const_buffers = {ConstantBuffer()};
    ShaderParameter space1 {};
    space1.const_buffers = {ConstantBuffer()};

    RenderTargetDesc rt_normal {ResourceFormat::R32G32B32A32Float};
    RenderTargetDesc rt_albedo {ResourceFormat::R8G8B8A8Unorm};
    render_target_descs = {rt_normal, rt_albedo};

    signature.param_table = {space0, space1};
  }
};

struct DeferredShadingMeta : RasterizationShaderMetaInfo {
  DeferredShadingMeta() {
    ShaderParameter space0 {};
    space0.const_buffers = {ConstantBuffer()};
    ShaderParameter space1 {};
    space1.const_buffers = {ConstantBuffer()};
    space1.shader_resources = {ShaderResource(), ShaderResource(), ShaderResource()};
    space1.static_samplers = {StaticSampler()};
    signature.param_table = {space0, space1};

    RenderTargetDesc rt_color {ResourceFormat::R8G8B8A8Unorm};
    render_target_descs = {rt_color};

    is_depth_stencil_disabled = true;
    merge.init_as_addictive();
  }
};

DeferredPipeline::DeferredPipeline(RendererPtr renderer) : SimplexPipeline(renderer) {
  Renderer* r = get_renderer();

  m_default_shader
    = r->create_shader(L"CoreData/shader/deferred_base.hlsl", std::make_unique<DeferredBaseMeta>());
  m_lighting_shader_base = r->create_shader(L"CoreData/shader/deferred_shading.hlsl",
    std::make_unique<DeferredShadingMeta>(), ShaderCompileConfig::defines<1>({{"BASE_SHADING", "1"},}));
  m_lighting_shader_add = r->create_shader(
    L"CoreData/shader/deferred_shading.hlsl", std::make_unique<DeferredShadingMeta>());

  {
    ConstBufferLayout lo = {
      ShaderDataType::Float4,   // screen size
      ShaderDataType::Float4x4, // camera view_proj
      ShaderDataType::Float4x4, // camera view_proj inverse
      ShaderDataType::Float4,   // camera pos
    };
    m_per_render_buffer = r->create_const_buffer(lo, 1, L"ShadingPass PerRender CB");
  }

  {
    ConstBufferLayout lo = {
      ShaderDataType::Float4, // light pos/dir
      ShaderDataType::Float4, // light color
    };
    m_per_light_buffer = r->create_const_buffer(lo, max_light_count, L"ShadingPass PerLight CB");
  }
}

DeferredPipeline::ViewportHandle DeferredPipeline::register_viewport(ViewportConfig conf) {
  Renderer* r = get_renderer();

  ViewportProxy proxy {};
  proxy.width = conf.width;
  proxy.height = conf.height;
  proxy.swapchain = r->create_swapchain(conf.window_id, conf.width, conf.height, 2);
  proxy.normal_buffer = r->create_texture_2d(
    conf.width, conf.height, ResourceFormat::R32G32B32A32Float, L"Normal Buffer");
  proxy.albedo_buffer = r->create_texture_2d(
    conf.width, conf.height, ResourceFormat::R8G8B8A8Unorm, L"Albedo Buffer");
  {
    TextureDesc ds_desc = TextureDesc::depth_stencil(conf.width, conf.height);
    proxy.depth_stencil_buffer
      = r->create_texture_2d(ds_desc, ResourceState::DeptpWrite, L"Depth Stencil");
    ShaderArgumentValue v {};
    v.const_buffers = {m_per_render_buffer};
    v.const_buffer_offsets = {0}; // assume only one viewport
    v.shader_resources = {proxy.depth_stencil_buffer, proxy.normal_buffer, proxy.albedo_buffer};
    proxy.shading_pass_per_render_arg = r->create_shader_argument(v);
  }
  {
    ShaderArgumentValue v {};
    v.const_buffers = {m_per_light_buffer};
    for (size_t i = 0; i < proxy.shading_pass_per_light_args.max_size; i++) {
      v.const_buffer_offsets = {i}; // assume only one viewport using the buffer at the same time
      proxy.shading_pass_per_light_args.push_back(r->create_shader_argument(v));
    }
  }

  return add_viewport(std::move(proxy));
}

void DeferredPipeline::transform_viewport(ViewportHandle handle, const Camera& camera) {
  ViewportProxy* viewport = get_viewport(handle);
  REI_ASSERT(viewport);
  Renderer* r = get_renderer();
  REI_ASSERT(r);

  viewport->cam_pos = camera.position();
  viewport->view_proj = r->is_depth_range_01() ? camera.view_proj_halfz() : camera.view_proj();
  viewport->view_proj_inv = viewport->view_proj.inv();
}

DeferredPipeline::SceneHandle DeferredPipeline::register_scene(SceneConfig conf) {
  const Scene* scene = conf.scene;
  Renderer* r = get_renderer();

  const int model_count = scene->get_models().size();

  SceneProxy proxy = {};
  {
    ConstBufferLayout cb_lo = {
      ShaderDataType::Float4x4, // WVP
      ShaderDataType::Float4x4, // World
    };
    proxy.objects_cb = r->create_const_buffer(cb_lo, model_count, L"Scene-Objects CB");
  }
  {
    for (auto& g : scene->geometries()) {
      auto geo = r->create_geometry({g});
      proxy.geometries.insert({scene->get_id(g), geo});
    }
  }
  {
    proxy.m_models.reserve(model_count);
    ShaderArgumentValue arg_value = {};
    { // init
      arg_value.const_buffers = {proxy.objects_cb};
      arg_value.const_buffer_offsets = {0};
    }
    for (size_t i = 0; i < model_count; i++) {
      auto model = scene->get_models()[i];
      auto* geo = proxy.geometries.try_get(scene->get_id(model->get_geometry()));
      arg_value.const_buffer_offsets[0] = i;
      ShaderArgumentHandle arg = r->create_shader_argument(arg_value);
      proxy.m_models.insert({
        scene->get_id(model), {*geo, arg, i, model->get_transform()} // value
      });
    }
  }

  return add_scene(std::move(proxy));
}

void DeferredPipeline::update_model(
  SceneHandle scene_handle, const Model& model, Scene::ModelUID model_id) {
  SceneProxy* scene = get_scene(scene_handle);
  REI_ASSERT(scene);
  auto* data = scene->m_models.try_get(model_id);
  data->trans = model.get_transform();
}

void DeferredPipeline::render(ViewportHandle viewport_h, SceneHandle scene_h) {
  ViewportProxy* viewport = get_viewport(viewport_h);
  SceneProxy* scene = get_scene(scene_h);
  REI_ASSERT(viewport);
  REI_ASSERT(scene);

  Renderer* const renderer = get_renderer();
  const auto cmd_list = renderer->prepare();

  // Update scene-wide const buffer (per-frame CB)
  {
    cmd_list->update_const_buffer(
      m_per_render_buffer, 0, 0, Vec4(viewport->width, viewport->height, 0, 0));
    cmd_list->update_const_buffer(m_per_render_buffer, 0, 1, viewport->view_proj);
    cmd_list->update_const_buffer(m_per_render_buffer, 0, 2, viewport->view_proj_inv);
    cmd_list->update_const_buffer(m_per_render_buffer, 0, 3, viewport->cam_pos);
  }

  // Update per-object const buffer
  {
    for (auto& pair : scene->m_models) {
      auto& model = pair.second;
      size_t index = model.cb_index;
      Mat4& m = model.trans;
      Mat4 mvp = viewport->view_proj * m;
      renderer->update_const_buffer(scene->objects_cb, index, 0, mvp);
      renderer->update_const_buffer(scene->objects_cb, index, 1, m);
    }
  }

  // Geometry pass
  cmd_list->transition(viewport->normal_buffer, ResourceState::RenderTarget);
  cmd_list->transition(viewport->albedo_buffer, ResourceState::RenderTarget);
  cmd_list->transition(viewport->depth_stencil_buffer, ResourceState::DeptpWrite);
  RenderPassCommand gpass = {};
  {
    gpass.render_targets = {viewport->normal_buffer, viewport->albedo_buffer};
    gpass.depth_stencil = viewport->depth_stencil_buffer;
    gpass.clear_ds = true;
    gpass.clear_rt = true;
    gpass.viewport = RenderViewaport::full(viewport->width, viewport->height);
    gpass.area = RenderArea::full(viewport->width, viewport->height);
  }
  cmd_list->begin_render_pass(gpass);
  {
    DrawCommand draw_cmd = {};
    draw_cmd.shader = m_default_shader;
    for (auto pair : scene->m_models) {
      auto& model = pair.second;
      draw_cmd.index_buffer = model.geometry.index_buffer;
      draw_cmd.vertex_buffer = model.geometry.vertex_buffer;
      draw_cmd.shader = m_default_shader;
      draw_cmd.arguments = {model.arg};
      cmd_list->draw(draw_cmd);
    }
  }
  cmd_list->end_render_pass();

  // Shading pass
  BufferHandle render_target = renderer->fetch_swapchain_render_target_buffer(viewport->swapchain);
  cmd_list->transition(render_target, ResourceState::RenderTarget);
  cmd_list->transition(viewport->normal_buffer, ResourceState::PixelShaderResource);
  cmd_list->transition(viewport->albedo_buffer, ResourceState::PixelShaderResource);
  cmd_list->transition(viewport->depth_stencil_buffer, ResourceState::PixelShaderResource);
  RenderPassCommand shading_pass = {};
  {
    shading_pass.render_targets = {render_target};
    shading_pass.depth_stencil = c_empty_handle;
    shading_pass.clear_rt = true;
    shading_pass.clear_ds = false;
    shading_pass.viewport = RenderViewaport::full(viewport->width, viewport->height);
    shading_pass.area = RenderArea::full(viewport->width, viewport->height);
  }
  cmd_list->begin_render_pass(shading_pass);
  std::vector<Vec4> light_pos = {{0.5, 0.6, 0.9, 0}, {0.5, 0.6, 0.8, 0}};
  for (int light_i = 0; light_i < 2; light_i++) {
    // Run a screen-pass for each light
    cmd_list->update_const_buffer(m_per_light_buffer, light_i, 0, light_pos[light_i]);      // pos
    cmd_list->update_const_buffer(m_per_light_buffer, light_i, 1, Vec4(0.7, 0.65, 0.6, 0)); // color

    DrawCommand cmd = {};
    cmd.shader = (light_i == 0) ? m_lighting_shader_base : m_lighting_shader_add;
    cmd.arguments
      = {viewport->shading_pass_per_light_args[light_i], viewport->shading_pass_per_render_arg};
    cmd_list->draw(cmd);
  }
  cmd_list->end_render_pass();

  cmd_list->transition(render_target, ResourceState::Present);
  cmd_list->present(viewport->swapchain, true);
}

} // namespace rei
