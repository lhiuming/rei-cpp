#if DIRECT3D_ENABLED
// Source of d3d_renderer.h
#include "d3d_renderer.h"

#include <unordered_set>

#include <d3d12.h>
#include <d3dx12.h>

#include "../debug.h"

#include "d3d_common_resources.h"
#include "d3d_device_resources.h"
#include "d3d_viewport_resources.h"

using std::make_shared;
using std::shared_ptr;
using std::unordered_set;
using std::vector;
using std::weak_ptr;

namespace rei {

namespace d3d {

// Default Constructor
Renderer::Renderer(HINSTANCE hinstance) : hinstance(hinstance) {
  device_resources = std::make_unique<DeviceResources>(hinstance);
  create_default_assets();
}

Renderer::~Renderer() {
  log("D3DRenderer is destructed.");
}

ViewportHandle Renderer::create_viewport(SystemWindowID window_id, int width, int height) {
  REI_ASSERT(window_id.platform == SystemWindowID::Win);

  HWND hwnd = window_id.value.hwnd;

  /*
   * NOTE: DirectX viewport starts from top-left to bottom-right.
   */

  D3D12_VIEWPORT d3d_vp {};
  d3d_vp.TopLeftX = 0.0f;
  d3d_vp.TopLeftY = 0.0f;
  d3d_vp.Width = width;
  d3d_vp.Height = height;
  d3d_vp.MinDepth = 0.0f;
  d3d_vp.MaxDepth = 1.0f;

  D3D12_RECT scissor {};
  scissor.left = 0;
  scissor.top = 0;
  scissor.right = width;
  scissor.bottom = height;

  auto vp_res = std::make_shared<ViewportResources>(device_resources->device,
    device_resources->dxgi_factory, device_resources->command_queue, hwnd, width, height);

  auto vp = std::make_shared<ViewportData>(this);
  vp->clear_color = {0.f, 0.f, 0.f, 0.5f};
  vp->d3d_viewport = d3d_vp;
  vp->viewport_resources = vp_res;
  vp->scissor = scissor;

  viewport_resources_lib.push_back(vp_res);
  return vp;
}

void Renderer::set_viewport_clear_value(ViewportHandle viewport_handle, Color c) {
  auto viewport = to_viewport(viewport_handle);
  viewport->clear_color = {c.r, c.g, c.b, c.a};
}

void Renderer::update_viewport_vsync(ViewportHandle viewport_handle, bool enabled_vsync) {
  auto viewport = to_viewport(viewport_handle);
  REI_ASSERT(viewport);
  viewport->enable_vsync = enabled_vsync;
}

void Renderer::update_viewport_size(ViewportHandle viewport, int width, int height) {
  error("Method is not implemented");
}

void Renderer::update_viewport_transform(ViewportHandle h_viewport, const Camera& camera) {
  shared_ptr<ViewportData> viewport = to_viewport(h_viewport);
  REI_ASSERT(viewport);
  viewport->update_camera_transform(camera);
}

ShaderHandle Renderer::create_shader(std::wstring shader_path) {
  auto shader = make_shared<ShaderData>(this);

  // compile and retrive root signature
  device_resources->compile_shader(shader_path, shader->compiled_data);
  device_resources->get_root_signature(shader->root_signature);
  device_resources->create_const_buffers(*shader, shader->const_buffers);

  return ShaderHandle(shader);
}

GeometryHandle Renderer::create_geometry(const Geometry& geometry) {
  // TODO currently only support mesh type
  auto& mesh = dynamic_cast<const Mesh&>(geometry);
  auto mesh_data = make_shared<MeshData>(this);
  device_resources->create_mesh_buffer(mesh, *mesh_data);
  return GeometryHandle(mesh_data);
}

ModelHandle Renderer::create_model(const Model& model) {
  if (REI_WARNING(model.get_geometry() == nullptr)) { return nullptr; }

  auto model_data = make_shared<ModelData>(this);

  // set up reference
  model_data->geometry = to_geometry(model.get_geometry()->get_graphic_handle());
  REI_ASSERT(model_data->geometry);
  if (!model.get_material()) {
    log("model has no material; use deafult mat");
    model_data->material = default_material;
  } else {
    model_data->material = to_material(model.get_material()->get_graphic_handle());
  }

  // allocate const buffer
  auto shader = to_shader(model_data->material->shader);
  REI_ASSERT(shader);
  ShaderConstBuffers& shader_cbs = shader->const_buffers;
  UINT cb_index = shader->const_buffers.next_object_index++;
  REI_ASSERT(cb_index < shader_cbs.per_object_CBs->size());
  model_data->const_buffer_index = cb_index;

  // allocate a cbv in the device shared heap
  ID3D12Resource* buffer = shader_cbs.per_object_CBs->resource();
  REI_ASSERT(buffer);
  D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
  cbv_desc.BufferLocation = shader_cbs.per_object_CBs->buffer_address(cb_index);
  cbv_desc.SizeInBytes = shader_cbs.per_object_CBs->element_bytesize();
  ID3D12DescriptorHeap* cb_heap = device_resources->shading_buffer_heap.Get();
  REI_ASSERT(cb_heap);
  UINT cbv_index = device_resources->next_shading_buffer_view_index++;
  REI_ASSERT(cbv_index < device_resources->max_shading_buffer_view_num);
  CD3DX12_CPU_DESCRIPTOR_HANDLE cbv {cb_heap->GetCPUDescriptorHandleForHeapStart(), (INT)cbv_index,
    device_resources->shading_buffer_view_inc_size};
  device_resources->device->CreateConstantBufferView(&cbv_desc, cbv);

  model_data->transform = model.get_transform(); // not necessary

  return ModelHandle(model_data);
}

void Renderer::prepare(Scene& scene) {
  // TODO update data from scene
  Scene::ModelsRef models = scene.get_models();
  for (ModelPtr& m : models) {
    shared_ptr<ModelData> data = to_model(m->get_rendering_handle());
    data->transform = m->get_transform();
  }

  // wait to finish reresource creation commands
  // TODO make this non-block
  upload_resources();
}

CullingResult Renderer::cull(ViewportHandle viewport_handle, const Scene& scene) {
  // TODO pool this
  auto culling_data = make_shared<CullingData>(this);
  vector<ModelData>& culled_models = culling_data->models;
  // TODO current culling is just a dummy process
  Scene::ModelsConstRef scene_models = scene.get_models();
  for (ModelPtr a : scene_models) {
    ModelHandle h_model = a->get_rendering_handle();
    if (warning_if(h_model == nullptr, "Model is not create in renderer")) { continue; }
    shared_ptr<ModelData> a_model = to_model(h_model);
    culled_models.push_back(*a_model);
  }

  // TODO check if need to draw debug
  if (draw_debug_model) {
    // add the debug model to culling result
    culled_models.push_back(*debug_model);
  }

  return CullingResult(culling_data);
}

void Renderer::render(const ViewportHandle viewport_handle, CullingResult culling_handle) {
  shared_ptr<ViewportData> p_viewport = to_viewport(viewport_handle);
  REI_ASSERT(p_viewport);
  shared_ptr<CullingData> p_culling = to_culling(culling_handle);
  REI_ASSERT(p_culling);
  render(*p_viewport, *p_culling);
}

void Renderer::upload_resources() {
  // NOTE: if the cmd_list is closed, we don need to submit it
  if (device_resources->is_uploading_reset) {
    auto upload_cmd_list = device_resources->upload_command_list;
    upload_cmd_list->Close();
    device_resources->is_uploading_reset = false;
    ID3D12CommandList* temp_cmd_lists[1] = {upload_cmd_list.Get()};
    device_resources->command_queue->ExecuteCommandLists(1, temp_cmd_lists);
  }
}

void Renderer::render(ViewportData& viewport, CullingData& culling) {
  REI_ASSERT(device_resources.get());
  REI_ASSERT(!viewport.viewport_resources.expired());

  auto& dev_res = *device_resources;
  auto& vp_res = *(viewport.viewport_resources.lock());

  // device shorcuts
  const ComPtr<ID3D12CommandAllocator> cmd_alloc = dev_res.command_alloc;
  const ComPtr<ID3D12GraphicsCommandList> cmd_list = dev_res.draw_command_list;
  const ComPtr<ID3D12CommandQueue> cmd_queue = dev_res.command_queue;

  // viewport shortcuts
  const D3D12_VIEWPORT& d3d_vp = viewport.d3d_viewport;
  const D3D12_RECT& scissor = viewport.scissor;
  const RenderTargetSpec& target_spec = vp_res.target_spec;

  // Prepare
  // TODO cannot reset allocator, because uplaod commands are not done, and they are using the same
  // allocator
  // ASSERT(SUCCEEDED(cmd_alloc->Reset())); // assume previous GPU tasks are finished
  if (!device_resources->is_drawing_reset) {
    REI_ASSERT(SUCCEEDED(cmd_list->Reset(
      cmd_alloc.Get(), NULL))); // reuse the command list object; fine with null init pso
  }

  // Prepare render target
  D3D12_RESOURCE_BARRIER pre_rt
    = CD3DX12_RESOURCE_BARRIER::Transition(vp_res.get_current_rt_buffer().Get(),
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

  // Draw opaque models
  // TODO sort the models into different level of batches, and draw by these batches
  ModelDrawTask r_task = {};
  r_task.view_proj = &viewport.view_proj;
  r_task.target_spec = &vp_res.target_spec;
  for (ModelData& model : culling.models) {
    if (REI_WARNING(model.geometry == nullptr) || REI_WARNING(model.material == nullptr)) continue;
    r_task.model_num = 1;
    r_task.models = &model;
    draw_meshes(r_task);
  }

  // Finish writing render target
  D3D12_RESOURCE_BARRIER post_rt
    = CD3DX12_RESOURCE_BARRIER::Transition(vp_res.get_current_rt_buffer().Get(),
      D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
  cmd_list->ResourceBarrier(1, &post_rt);

  // Finish adding commands
  REI_ASSERT(SUCCEEDED(cmd_list->Close()));
  dev_res.is_drawing_reset = false;

  // Submit the only command list
  ID3D12CommandList* temp_cmd_lists[1] = {cmd_list.Get()};
  dev_res.command_queue->ExecuteCommandLists(1, temp_cmd_lists);
  // cmd_list.Reset(); // fine to reset it no

  // Present and flip
  if (viewport.enable_vsync) {
    vp_res.swapchain->Present(1, 0);
  } else {
    vp_res.swapchain->Present(0, 0);
  }
  vp_res.flip_backbuffer();

  // Flush and wait
  dev_res.flush_command_queue_for_frame();

  REI_ASSERT(SUCCEEDED(cmd_alloc->Reset())); // assume previous GPU tasks are finished
}

void Renderer::create_default_assets() {
  // default mesh
  MeshPtr cube = make_shared<Mesh>(L"D3D-Renderer Debug Cube");
  vector<Mesh::Vertex> vertices = {
    //     Position               Normal
    {{-1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f}},
    {{-1.0f, +1.0f, -1.0f}, {-1.0f, +1.0f, -1.0f}},
    {{+1.0f, +1.0f, -1.0f}, {+1.0f, +1.0f, -1.0f}},
    {{+1.0f, -1.0f, -1.0f}, {+1.0f, -1.0f, -1.0f}},
    {{-1.0f, -1.0f, +1.0f}, {-1.0f, -1.0f, +1.0f}},
    {{-1.0f, +1.0f, +1.0f}, {-1.0f, +1.0f, +1.0f}},
    {{+1.0f, +1.0f, +1.0f}, {+1.0f, +1.0f, +1.0f}},
    {{+1.0f, -1.0f, +1.0f}, {+1.0f, -1.0f, +1.0f}},
  };
  vector<Mesh::Triangle> indicies = {// front face
    {0, 1, 2}, {0, 2, 3},

    // back face
    {4, 6, 5}, {4, 7, 6},

    // left face
    {4, 5, 1}, {4, 1, 0},

    // right face
    {3, 2, 6}, {3, 6, 7},

    // top face
    {1, 5, 6}, {1, 6, 2},

    // bottom face
    {4, 0, 3}, {4, 3, 7}};
  cube->set(std::move(vertices), std::move(indicies));
  MaterialPtr mat = make_shared<Material>();

  // default mesh
  auto default_mesh = make_shared<MeshData>(this);
  device_resources->create_mesh_buffer(*cube, *default_mesh);
  default_geometry = default_mesh;
  cube->set_graphic_handle(default_geometry);

  // default material & shader
  default_shader = to_shader(create_shader(L"CoreData/shader/shader.hlsl"));
  default_material = make_shared<MaterialData>(this);
  default_material->shader = default_shader;
  mat->set_graphic_handle(default_material);

  // combining the default assets
  Model central_cube {L"Central Cubeeee", Mat4::I(), cube, mat};
  debug_model = to_model(create_model(central_cube));
}

void Renderer::draw_meshes(ModelDrawTask& task) {
  // short cuts
  ComPtr<ID3D12GraphicsCommandList> cmd_list = device_resources->draw_command_list;
  const Mat4& view_proj_mat = *(task.view_proj);
  const RenderTargetSpec& target_spec = *(task.target_spec);

  // shared setup
  cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  const ModelData* models = task.models;
  ID3D12RootSignature* curr_root_sign = nullptr;
  ID3D12PipelineState* curr_pso = nullptr;
  for (std::size_t i = 0; i < task.model_num; i++) {
    const ModelData& model = models[i];
    const MeshData& mesh = *(static_cast<MeshData*>(model.geometry.get()));
    const MaterialData& material = *(model.material);
    const ShaderData& shader = *(material.shader);
    const ShaderConstBuffers& const_buffers = shader.const_buffers;

    // Check root signature
    ID3D12RootSignature* this_root_sign = shader.root_signature.Get();
    if (curr_root_sign != this_root_sign) {
      cmd_list->SetGraphicsRootSignature(this_root_sign);
      curr_root_sign = this_root_sign;
    }

    // Check Pipeline state
    ComPtr<ID3D12PipelineState> this_pso;
    device_resources->get_pso(shader, target_spec, this_pso);
    if (curr_pso != this_pso.Get()) {
      cmd_list->SetPipelineState(this_pso.Get());
      curr_pso = this_pso.Get();
    }

    // Bind parameters
    // Per frame data // TODO check redudant
    cmd_list->SetGraphicsRootConstantBufferView(1, const_buffers.per_frame_CB->buffer_address());
    // Per object data
    cbPerObject object_cb = {};
    object_cb.update(model.transform * view_proj_mat, model.transform);
    const_buffers.per_object_CBs->update(object_cb, model.const_buffer_index);
    cmd_list->SetGraphicsRootConstantBufferView(
      0, const_buffers.per_object_CBs->buffer_address(model.const_buffer_index));

    // Set geometry buffer
    cmd_list->IASetVertexBuffers(0, 1, &mesh.vbv);
    cmd_list->IASetIndexBuffer(&mesh.ibv);

    // Draw
    cmd_list->DrawIndexedInstanced(mesh.index_num, 1, 0, 0, 0);
  } // end for each model
}

} // namespace d3d

} // namespace rei

#endif