#include "deferred.h"

#include <memory>
#include <unordered_map>

#include "../direct3d/d3d_renderer.h"

namespace rei {

namespace deferred {
struct ViewportData {
  size_t width = 1;
  size_t height = 1;
  SwapchainHandle swapchain;
  ShaderArgumentHandle lighting_pass_argument;
  Mat4 view_proj = Mat4::I();
  Vec4 cam_pos = {0, 1, 8, 1};
};
struct SceneData {
  BufferHandle const_buf;
  // holding for default material
  BufferHandle objects_const_buf;
  struct ModelData {
    ModelHandle model;
    GeometryHandle geometry;
    // TODO should use an offset in cb instead, rather than a brand-new descriptor
    ShaderArgumentHandle arg;
    Mat4 trans;
  };
  std::vector<ModelData> models;
};
} // namespace deferred

using ViewportProxy = deferred::ViewportData;
using SceneProxy = deferred::SceneData;

struct DeferredBaseMeta : RasterizationShaderMetaInfo {
  DeferredBaseMeta() {
    ShaderParameter space0 {};
    space0.const_buffers = {ConstBuffer()};
    ShaderParameter space1 {};
    space1.const_buffers = {ConstBuffer()};
    signature.param_table = {space0, space1};
  }
};

struct DeferredShadingMeta : RasterizationShaderMetaInfo {
  DeferredShadingMeta() {
    ShaderParameter space0 {};
    space0.shader_resources = {ShaderResource()};
    signature.param_table = {space0};
    is_depth_stencil_disabled = true;
  }
};

DeferredPipeline::DeferredPipeline(RendererPtr renderer) : SimplexPipeline(renderer) {
  Renderer* r = get_renderer();
  m_default_shader
    = r->create_shader(L"CoreData/shader/deferred_base.hlsl", std::make_unique<DeferredBaseMeta>());
  m_lighting_shader
    = r->create_shader(L"CoreData/shader/deferred_shading.hlsl", std::make_unique<DeferredShadingMeta>());
}

DeferredPipeline::ViewportHandle DeferredPipeline::register_viewport(ViewportConfig conf) {
  Renderer* r = get_renderer();

  ViewportProxy proxy {};
  proxy.width = conf.width;
  proxy.height = conf.height;
  proxy.swapchain = r->create_swapchain(conf.window_id, conf.width, conf.height, 2);
  {
    auto ds_buf = r->fetch_swapchain_depth_stencil_buffer(proxy.swapchain);
    ShaderArgumentValue v {};
    v.shader_resources.push_back(ds_buf);
    proxy.lighting_pass_argument = r->create_shader_argument(m_lighting_shader, v);
  }

  return add_viewport(std::move(proxy));
}

DeferredPipeline::SceneHandle DeferredPipeline::register_scene(SceneConfig conf) {
  const Scene* scene = conf.scene;
  Renderer* r = get_renderer();

  const int model_count = scene->get_models().size();

  SceneProxy proxy = {};
  {
    ConstBufferLayout lo = {
      ShaderDataType::Float4x4,
      ShaderDataType::Float4,
    };
    proxy.const_buf = r->create_const_buffer(lo, 1);
  }
  {
    ConstBufferLayout cb_lo = {
      ShaderDataType::Float4x4, // WVP
      ShaderDataType::Float4x4, // World
    };
    proxy.objects_const_buf = r->create_const_buffer(cb_lo, model_count);
  }
  {
    proxy.models.reserve(model_count);
    ShaderArgumentValue arg_value = {};
    arg_value.const_buffers.push_back(proxy.objects_const_buf);
    arg_value.const_buffer_offsets.push_back(-1);
    for (size_t i = 0; i < scene->get_models().size(); i++) {
      auto model = scene->get_models()[i];
      arg_value.const_buffer_offsets[0] = i;
      ShaderArgumentHandle arg = r->create_shader_argument(m_default_shader, arg_value);
      proxy.models.push_back({model->get_rendering_handle(),
        model->get_geometry()->get_graphic_handle(), arg, model->get_transform()});
    }
  }

  return add_scene(std::move(proxy));
}

void DeferredPipeline::transform_viewport(ViewportHandle handle, const Camera& camera) {
  ViewportProxy* viewport = get_viewport(handle);
  REI_ASSERT(viewport);
  Renderer* r = get_renderer();
  REI_ASSERT(r);

  viewport->cam_pos = camera.position();
  viewport->view_proj = r->is_depth_range_01() ? camera.view_proj_halfz() : camera.view_proj();
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
    cmd_list->update_const_buffer(scene->const_buf, 0, 0, viewport->view_proj);
    cmd_list->update_const_buffer(scene->const_buf, 0, 1, viewport->cam_pos);
  }

  // Update per-object const buffer
  {
    for (int i = 0, n = scene->models.size(); i < n; i++) {
      Mat4& m = scene->models[i].trans;
      Mat4 mvp = viewport->view_proj * m;
      renderer->update_const_buffer(scene->objects_const_buf, i, 0, mvp);
      renderer->update_const_buffer(scene->objects_const_buf, i, 1, m);
    }
  }

  // Prepare frame buffer
  BufferHandle ds_buffer = renderer->fetch_swapchain_depth_stencil_buffer(viewport->swapchain);

  cmd_list->transition(ds_buffer, ResourceState::DeptpWrite);
  cmd_list->begin_render_pass(c_empty_handle, ds_buffer, {viewport->width, viewport->height}, false,
    true);
  {
    DrawCommand draw_cmd = {};
    draw_cmd.shader = m_default_shader;
    size_t n = scene->models.size();
    for (size_t i = 0; i < n; i++) {
      auto& model = scene->models[i];
      draw_cmd.geo = model.geometry;
      draw_cmd.shader = m_default_shader;
      draw_cmd.arguments = {model.arg};
      cmd_list->draw(draw_cmd);
    }
  }
  cmd_list->end_render_pass();


  BufferHandle render_target = renderer->fetch_swapchain_render_target_buffer(viewport->swapchain);
  cmd_list->transition(render_target, ResourceState::RenderTarget);
  cmd_list->begin_render_pass(
    render_target, c_empty_handle, {viewport->width, viewport->height}, true, false);
  {
    DrawCommand cmd = {};
    cmd.geo = c_empty_handle;
    cmd.shader = m_lighting_shader;
    cmd.arguments = {viewport->lighting_pass_argument};
    cmd_list->draw(cmd);
  }
  cmd_list->end_render_pass();

  cmd_list->transition(render_target, ResourceState::Present);
  cmd_list->present(viewport->swapchain, true);
}

} // namespace rei
