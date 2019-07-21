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
    // TODO should use an offset in cb instead
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
  DeferredShadingMeta() {}
};

DeferredPipeline::DeferredPipeline(RendererPtr renderer) : SimplexPipeline(renderer) {
  Renderer* r = get_renderer();
  m_default_shader
    = r->create_shader(L"CoreData/shader/deferred_base", std::make_unique<DeferredBaseMeta>());
  m_lighting_shader
    = r->create_shader(L"CoreData/shader/deferred_shading", std::make_unique<DeferredBaseMeta>());
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
  BufferHandle render_target = renderer->fetch_swapchain_render_target_buffer(viewport->swapchain);
  BufferHandle ds_buffer = renderer->fetch_swapchain_depth_stencil_buffer(viewport->swapchain);
  cmd_list->transition(render_target, ResourceState::RenderTarget);
  cmd_list->transition(ds_buffer, ResourceState::DeptpWrite);

  cmd_list->begin_render_pass(render_target, ds_buffer, {viewport->width, viewport->height}, true,
    true); // rendertarget, depth-stencil, render area, clear values

  /*

  // Set Viewport and scissor
  cmd_list->RSSetViewports(1, &d3d_vp);
  cmd_list->RSSetScissorRects(1, &scissor);

  // Specify render target
  cmd_list->OMSetRenderTargets(1, &vp_res.get_current_rtv(), true, &vp_res.get_dsv());

  // Clear
  const FLOAT* clear_color = viewport.clear_color.data();
  D3D12_RECT* entire_view = nullptr;
  cmd_list->ClearRenderTargetView(vp_res.get_current_rtv(), clear_color, 0, entire_view);
  D3D12_CLEAR_FLAGS ds_clear_flags = D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL;
  FLOAT clear_depth = target_spec.ds_clear.Depth;
  FLOAT clear_stencil = target_spec.ds_clear.Stencil;
  cmd_list->ClearDepthStencilView(
    vp_res.get_dsv(), ds_clear_flags, clear_depth, clear_stencil, 0, entire_view);
  */

  {
    DrawCommand draw_cmd = {};
    draw_cmd.shader = m_default_shader;
    size_t n = scene->models.size();
    for (size_t i = 0; i < n; i++) {
      auto& model = scene->models[i];
      draw_cmd.geo = model.geometry;
      draw_cmd.shader = m_default_shader;
      draw_cmd.arguments = {model.arg};
      // with geometry handles, shader argumnets and pipeline state /w root signature
      cmd_list->draw(draw_cmd);
    }
  }

  cmd_list->end_render_pass();

  /*
  // Geomtry Pass
  // TODO sort the models into different level of batches, and draw by these batches
  {
    ModelDrawTask r_task = {};
    r_task.view_proj = &viewport.view_proj;
    r_task.target_spec = &target_spec;
    for (ModelData& model : culling.models) {
      if (REI_WARNINGIF(model.geometry == nullptr) || REI_WARNINGIF(model.material == nullptr))
        continue;
      r_task.model_num = 1;
      r_task.models = &model;
      draw_meshes(*cmd_list, r_task, to_shader(deferred_base_pass).get());
    }
  }
  */

  {
    BufferHandle rt = renderer->fetch_swapchain_render_target_buffer(viewport->swapchain);
    BufferHandle ds = c_empty_handle;
    cmd_list->begin_render_pass(rt, ds, {viewport->width, viewport->height}, true, false);
  }

  {
    DrawCommand cmd = {};
    cmd.geo = c_empty_handle;
    cmd.shader = m_lighting_shader;
    cmd.arguments = {viewport->lighting_pass_argument};
    // with null-geometry, shader arguments, and pipeline state /w root signature
    cmd_list->draw(cmd);
  }

  cmd_list->end_render_pass();

  /*
  // Copy depth to render target for test
  {
    auto& shader = to_shader(deferred_shading_pass);

    cmd_list->OMSetRenderTargets(1, &vp_res.get_current_rtv(), true, NULL);

    cmd_list->SetGraphicsRootSignature(shader->root_signature.Get());
    ComPtr<ID3D12PipelineState> pso;
    device_resources->get_pso(*shader, target_spec, pso);
    cmd_list->SetPipelineState(pso.Get());

    cmd_list->SetGraphicsRootDescriptorTable(
      DeferredShadingMeta::GBufferTable, viewport.depth_buffer_srv_gpu);

    cmd_list->IASetVertexBuffers(0, 0, NULL);
    cmd_list->IASetIndexBuffer(NULL);
    cmd_list->DrawInstanced(6, 1, 0, 0);
  }
  */

  cmd_list->present(viewport->swapchain, true);
}

} // namespace rei
