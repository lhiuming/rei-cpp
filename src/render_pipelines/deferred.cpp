#include "deferred.h"

#include "../direct3d/d3d_renderer.h"

namespace rei {

using ViewportProxy = deferred::ViewportData;
using SceneProxy = deferred::SceneData;

// FIXME lazy
using Renderer = d3d::Renderer;

DeferredPipeline::DeferredPipeline(RendererPtr renderer) : SimplexPipeline(renderer) {
  // todo init
}

void DeferredPipeline::render(ViewportHandle viewport_h, SceneHandle scene_h) {
  ViewportProxy* viewport = get_viewport(viewport_h);
  SceneProxy* scene = get_scene(scene_h);
  REI_ASSERT(viewport);
  REI_ASSERT(scene);

  Renderer* const renderer = get_renderer();
  Renderer* const cmd_list = renderer;

  cmd_list->begin_render();

  /*
  // Prepare command list
  // Prepare render target
  D3D12_RESOURCE_BARRIER pre_rt
    = CD3DX12_RESOURCE_BARRIER::Transition(vp_res.get_current_rt_buffer(),
      D3D12_RESOURCE_STATE_PRESENT, // previous either COMMON of PRESENT
      D3D12_RESOURCE_STATE_RENDER_TARGET);
  cmd_list->ResourceBarrier(1, &pre_rt);

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

  // Update per-frame buffers for all shaders
  // TODO cache this object
  unordered_set<UploadBuffer<cbPerFrame>*> per_frame_CBs {};
  for (ModelData& model : culling.models) {
    ShaderData* shader = model.material->shader.get();
    REI_ASSERT(shader);
    REI_ASSERT(shader->const_buffers.per_frame_CB.get());
    per_frame_CBs.insert(shader->const_buffers.per_frame_CB.get());
  }
  cbPerFrame frame_cb = {};
  frame_cb.light = {};
  frame_cb.set_camera_world_trans(viewport.view);
  frame_cb.set_camera_pos(viewport.pos);
  for (UploadBuffer<cbPerFrame>* cb : per_frame_CBs) {
    cb->update(frame_cb);
  }

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

  // Finish writing render target
  D3D12_RESOURCE_BARRIER post_rt
    = CD3DX12_RESOURCE_BARRIER::Transition(vp_res.get_current_rt_buffer(),
      D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
  cmd_list->ResourceBarrier(1, &post_rt);
  */

  cmd_list->present(viewport->swapchain, false);
  cmd_list->end_render();
}

} // namespace rei
