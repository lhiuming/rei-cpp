#if DIRECT3D_ENABLED
// Source of d3d_renderer.h
#include "d3d_renderer.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_set>

#include <d3d12.h>
#include <d3dx12.h>

#include "../debug.h"
#include "../string_utils.h"

#include "d3d_common_resources.h"
#include "d3d_device_resources.h"
#include "d3d_utils.h"
#include "d3d_viewport_resources.h"

using std::make_shared;
using std::shared_ptr;
using std::unordered_set;
using std::vector;
using std::weak_ptr;

namespace rei {

namespace d3d {

namespace dxr {
/*
 * Hard code the root siagnature for dxr
 * FIXME
 */
namespace GlobalRootSignLayout {
enum Data { OuputTextureUAV_SingleTable = 0, TLAS_SRV, Count };
}
namespace LocalRootSignLayout {
enum Data { ViewportConstants = 0, Count };

} // namespace LocalRootSignLayout

struct RaygenConstantBuffer {
  float x, y, z, w;
};

const wchar_t* c_hit_group_name = L"hit_group0";
const wchar_t* c_raygen_shader_name = L"raygen_shader";
const wchar_t* c_closest_hit_shader_name = L"closest_hit_shader";
const wchar_t* c_miss_shader_name = L"miss_shader";

} // namespace dxr

// Default Constructor
Renderer::Renderer(HINSTANCE hinstance, Options opt)
    : hinstance(hinstance),
      mode(opt.init_render_mode),
      is_dxr_enabled(opt.enable_realtime_raytracing) {
  DeviceResources::Options dev_opt;
  dev_opt.is_dxr_enabled = is_dxr_enabled;
  device_resources = std::make_shared<DeviceResources>(hinstance, dev_opt);
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
  d3d_vp.MinDepth = D3D12_MIN_DEPTH;
  d3d_vp.MaxDepth = D3D12_MAX_DEPTH;

  D3D12_RECT scissor {};
  scissor.left = 0;
  scissor.top = 0;
  scissor.right = width;
  scissor.bottom = height;

  auto vp_res = std::make_shared<ViewportResources>(device_resources, hwnd, width, height);

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
  if (REI_WARNINGIF(model.get_geometry() == nullptr)) { return nullptr; }

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

  /*
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
  device_resources->m_device->CreateConstantBufferView(&cbv_desc, cbv);
  */

  model_data->transform = model.get_transform(); // not necessary

  return ModelHandle(model_data);
}

SceneHandle Renderer::build_enviroment(const Scene& scene) {
  auto scene_data = make_shared<SceneData>(this);

  build_raytracing_rootsignatures();

  build_raytracing_pso();

  build_dxr_acceleration_structure(nullptr, 0);

  build_shader_table();

  return SceneHandle(scene_data);
}

void Renderer::prepare(Scene& scene) {
  // TODO update data from scene
  Scene::ModelsRef models = scene.get_models();
  for (ModelPtr& m : models) {
    shared_ptr<ModelData> data = to_model(m->get_rendering_handle());
    data->update_transform(*m);
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

  if (mode == RenderMode::Rasterization) {
    render(*p_viewport, *p_culling);
  } else {
    raytracing(*p_viewport, *p_culling);
  }
}

void Renderer::upload_resources() {
  // NOTE: if the cmd_list is closed, we don need to submit it
  if (is_uploading_resources) {
    device_resources->flush_command_list();
    is_uploading_resources = false;
  }
}

void Renderer::render(ViewportData& viewport, CullingData& culling) {
  REI_ASSERT(device_resources.get());
  REI_ASSERT(!viewport.viewport_resources.expired());

  auto& dev_res = *device_resources;
  auto& vp_res = *(viewport.viewport_resources.lock());

  // viewport shortcuts
  const D3D12_VIEWPORT& d3d_vp = viewport.d3d_viewport;
  const D3D12_RECT& scissor = viewport.scissor;
  const RenderTargetSpec& target_spec = vp_res.target_spec();

  // Prepare command list
  ID3D12GraphicsCommandList* cmd_list = dev_res.prepare_command_list();

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

  // Draw opaque models
  // TODO sort the models into different level of batches, and draw by these batches
  ModelDrawTask r_task = {};
  r_task.view_proj = &viewport.view_proj;
  r_task.target_spec = &target_spec;
  for (ModelData& model : culling.models) {
    if (REI_WARNINGIF(model.geometry == nullptr) || REI_WARNINGIF(model.material == nullptr))
      continue;
    r_task.model_num = 1;
    r_task.models = &model;
    draw_meshes(*cmd_list, r_task);
  }

  // Finish writing render target
  D3D12_RESOURCE_BARRIER post_rt
    = CD3DX12_RESOURCE_BARRIER::Transition(vp_res.get_current_rt_buffer(),
      D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
  cmd_list->ResourceBarrier(1, &post_rt);

  // Finish adding commands
  device_resources->flush_command_list();

  // Present and flip
  if (viewport.enable_vsync) {
    vp_res.swapchain()->Present(1, 0);
  } else {
    vp_res.swapchain()->Present(0, 0);
  }
  vp_res.flip_backbuffer();

  // Flush and wait
  dev_res.flush_command_queue_for_frame();
}

void Renderer::raytracing(ViewportData& viewport, CullingData& culling) {
  auto cmd_list = device_resources->prepare_command_list_dxr();

  auto viewport_resources = viewport.viewport_resources.lock();

  // Dispatch ray and write to raytracing output
  {
    cmd_list->SetComputeRootSignature(device_resources->global_root_sign.Get());
    cmd_list->SetDescriptorHeaps(1, device_resources->descriptor_heap_ptr());
    cmd_list->SetPipelineState1(device_resources->dxr_pso.Get());

    // Bind global root arguments
    cmd_list->SetComputeRootDescriptorTable(dxr::GlobalRootSignLayout::OuputTextureUAV_SingleTable, viewport_resources->raytracing_output_gpu_uav());
    cmd_list->SetComputeRootShaderResourceView(
      dxr::GlobalRootSignLayout::TLAS_SRV, this->tlas_buffer->GetGPUVirtualAddress());

    // Dispatch
    D3D12_DISPATCH_RAYS_DESC desc = {};
    desc.RayGenerationShaderRecord.StartAddress = raygen_shader_table->GetGPUVirtualAddress();
    desc.RayGenerationShaderRecord.SizeInBytes = raygen_shader_table->GetDesc().Width;
    desc.MissShaderTable.StartAddress = miss_shader_table->GetGPUVirtualAddress();
    desc.MissShaderTable.SizeInBytes = miss_shader_table->GetDesc().Width;
    desc.MissShaderTable.StrideInBytes = miss_shader_table->GetDesc().Width; // sinlge element
    desc.HitGroupTable.StartAddress = hitgroup_shader_table->GetGPUVirtualAddress();
    desc.HitGroupTable.SizeInBytes = hitgroup_shader_table->GetDesc().Width;
    desc.HitGroupTable.StrideInBytes = hitgroup_shader_table->GetDesc().Width; // single element
    desc.Width = viewport_resources->raytracing_output_width();
    desc.Height = viewport_resources->raytracing_output_height();
    desc.Depth = 1;
    cmd_list->DispatchRays(&desc);
  }

  // Copy to frame buffer
  {
    auto render_target = viewport_resources->get_current_rt_buffer();
    auto raytracing_output = viewport_resources->raytracing_output_buffer();

    D3D12_RESOURCE_BARRIER pre_copy[2];
    pre_copy[0] = CD3DX12_RESOURCE_BARRIER::Transition(
      raytracing_output, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    pre_copy[1] = CD3DX12_RESOURCE_BARRIER::Transition(
      render_target, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
    cmd_list->ResourceBarrier(2, pre_copy);

    cmd_list->CopyResource(render_target, raytracing_output);

    D3D12_RESOURCE_BARRIER post_copy[2];
    post_copy[0] = CD3DX12_RESOURCE_BARRIER::Transition(raytracing_output, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    post_copy[1] = CD3DX12_RESOURCE_BARRIER::Transition(render_target, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
    cmd_list->ResourceBarrier(2, post_copy);
  }

  // Done
  device_resources->flush_command_list();

 // Present and flip
  if (viewport.enable_vsync) {
    viewport_resources->swapchain()->Present(1, 0);
  } else {
    viewport_resources->swapchain()->Present(0, 0);
  }
  viewport_resources->flip_backbuffer();

  // Wait
  device_resources->flush_command_queue_for_frame();
}

void Renderer::create_default_assets() {
  REI_ASSERT(is_right_handed);
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

void Renderer::draw_meshes(ID3D12GraphicsCommandList& cmd_list, ModelDrawTask& task) {
  // short cuts
  const Mat4& view_proj_mat = *(task.view_proj);
  const RenderTargetSpec& target_spec = *(task.target_spec);

  // shared setup
  cmd_list.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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
      cmd_list.SetGraphicsRootSignature(this_root_sign);
      curr_root_sign = this_root_sign;
    }

    // Check Pipeline state
    ComPtr<ID3D12PipelineState> this_pso;
    device_resources->get_pso(shader, target_spec, this_pso);
    if (curr_pso != this_pso.Get()) {
      cmd_list.SetPipelineState(this_pso.Get());
      curr_pso = this_pso.Get();
    }

    // Bind parameters
    // Per frame data // TODO check redudant
    cmd_list.SetGraphicsRootConstantBufferView(1, const_buffers.per_frame_CB->buffer_address());
    // Per object data
    cbPerObject object_cb = {};
    REI_ASSERT(is_right_handed);
    object_cb.update(view_proj_mat * model.transform, model.transform);
    const_buffers.per_object_CBs->update(object_cb, model.const_buffer_index);
    cmd_list.SetGraphicsRootConstantBufferView(
      0, const_buffers.per_object_CBs->buffer_address(model.const_buffer_index));

    // Set geometry buffer
    cmd_list.IASetVertexBuffers(0, 1, &mesh.vbv);
    cmd_list.IASetIndexBuffer(&mesh.ibv);

    // Draw
    cmd_list.DrawIndexedInstanced(mesh.index_num, 1, 0, 0, 0);
  } // end for each model
}

void Renderer::build_raytracing_rootsignatures() {
  // Global root signature
  {
    constexpr int param_count = static_cast<int>(dxr::GlobalRootSignLayout::Count);
    CD3DX12_ROOT_PARAMETER root_params[param_count] = {};
    CD3DX12_DESCRIPTOR_RANGE output_uav(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    root_params[dxr::GlobalRootSignLayout::OuputTextureUAV_SingleTable].InitAsDescriptorTable(
      1, &output_uav); // Texture must be bound through table
    root_params[dxr::GlobalRootSignLayout::TLAS_SRV].InitAsShaderResourceView(0);
    CD3DX12_ROOT_SIGNATURE_DESC desc(param_count, root_params);
    device_resources->create_root_signature(desc, device_resources->global_root_sign);
  }

  // Local root signature
  {
    constexpr int param_count = static_cast<int>(dxr::LocalRootSignLayout::Count);
    CD3DX12_ROOT_PARAMETER root_params[param_count];
    root_params[dxr::LocalRootSignLayout::ViewportConstants].InitAsConstants(
      sizeof(dxr::RaygenConstantBuffer) / sizeof(int32_t), 0, 0);
    CD3DX12_ROOT_SIGNATURE_DESC desc(ARRAYSIZE(root_params), root_params);
    desc.Flags
      = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE; // special flag for dxr local root signature
    device_resources->create_root_signature(desc, device_resources->local_root_sign);
  }
}

void Renderer::build_raytracing_pso() {
  // hard-coded properties
  const std::wstring shader_path = L"CoreData/shader/raytracing.hlsl";
  const UINT payload_max_size = sizeof(float) * 4;
  const UINT attr_max_size = sizeof(float) * 2;
  const UINT max_raytracing_recursion_depth = 2;

  // short cut
  auto device = device_resources->dxr_device();

  CD3DX12_STATE_OBJECT_DESC raytracing_pipeline_desc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

  ComPtr<IDxcBlob> shader_blobs[3];
  D3D12_SHADER_BYTECODE bytecode[3];
  { // compile and bind the shaders
    const wchar_t* names[3]
      = {dxr::c_raygen_shader_name, dxr::c_closest_hit_shader_name, dxr::c_miss_shader_name};
    for (int i = 0; i < 3; i++) {
      auto dxil_lib = raytracing_pipeline_desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
      shader_blobs[i]
        = compile_dxr_shader(shader_path.data(), names[i]); // TODO the entry point may be incalid
      bytecode[i] = CD3DX12_SHADER_BYTECODE(
        shader_blobs[i]->GetBufferPointer(), shader_blobs[i]->GetBufferSize());
      dxil_lib->SetDXILLibrary(&bytecode[i]);
      dxil_lib->DefineExport(names[i]);
    }
  }

  { // setup hit group
    auto hit_group = raytracing_pipeline_desc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    hit_group->SetClosestHitShaderImport(
      dxr::c_closest_hit_shader_name); // not any-hit or intersection shader
    hit_group->SetHitGroupExport(dxr::c_hit_group_name);
    hit_group->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
  }

  { // DXR shader config
    auto shader_config
      = raytracing_pipeline_desc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    shader_config->Config(payload_max_size, attr_max_size);
  }

  { // bind local root signature and association
    auto local_root
      = raytracing_pipeline_desc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    auto sign = device_resources->local_root_sign;
    local_root->SetRootSignature(sign.Get());

    auto rootSignatureAssociation
      = raytracing_pipeline_desc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    rootSignatureAssociation->SetSubobjectToAssociate(*local_root);
    rootSignatureAssociation->AddExport(dxr::c_raygen_shader_name);
  }

  { // bind global root signature
    auto global_root
      = raytracing_pipeline_desc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    auto sign = device_resources->global_root_sign;
    global_root->SetRootSignature(sign.Get());
  }

  { // pipeline config
    auto pso_config
      = raytracing_pipeline_desc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    pso_config->Config(max_raytracing_recursion_depth);
  }

  HRESULT hr
    = device->CreateStateObject(raytracing_pipeline_desc, IID_PPV_ARGS(&device_resources->dxr_pso));
  REI_ASSERT(SUCCEEDED(hr));
}

void Renderer::build_dxr_acceleration_structure(ModelData* models, int count) {
  auto device = device_resources->dxr_device();
  auto cmd_list = device_resources->prepare_command_list_dxr();

  // Create a triangle for test
  struct Vec3f {
    float x, y, z;
  };
  {
    UINT16 indices[] = {0, 1, 2};
    float depthValue = 1.0;
    float offset = 0.7f;
    Vec3f vertices[]
      = {{0, -offset, depthValue}, {-offset, offset, depthValue}, {offset, offset, depthValue}};
    index_buffer = create_upload_buffer(device, indices, 3);
    vertex_buffer = create_upload_buffer(device, vertices, 3);
  }

  // Collect all model/geometry-instances in the raytracing scene
  std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geo_descs;
  {
    // TODO only support mesh currently

    // ModelData& m = *models;
    // MeshData& mesh = static_cast<MeshData&>(*m.geometry);
    bool is_opaque = true;

    D3D12_RAYTRACING_GEOMETRY_DESC geo_desc {};
    geo_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geo_desc.Triangles.Transform3x4 = NULL; // not used TODO check this
    geo_desc.Triangles.IndexFormat = c_index_format;
    geo_desc.Triangles.VertexFormat = c_accel_struct_vertex_format;
    geo_desc.Triangles.IndexCount = 3;
    geo_desc.Triangles.VertexCount = 3;
    geo_desc.Triangles.IndexBuffer = index_buffer->GetGPUVirtualAddress();
    geo_desc.Triangles.VertexBuffer.StartAddress = vertex_buffer->GetGPUVirtualAddress();
    geo_desc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vec3f);
    if (is_opaque) geo_desc.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    geo_descs.push_back(geo_desc);
  }

  int instance_count = 1;

  // Prepare: get the prebuild info for BLAS and TLAS
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tl_prebuild = {};
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS toplevel_input = {};
  {
    toplevel_input.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    toplevel_input.Flags
      = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE; // High quality
    toplevel_input.NumDescs = instance_count;
    toplevel_input.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    toplevel_input.pGeometryDescs = NULL; // instance desc in GPU memory; set latter
    device->GetRaytracingAccelerationStructurePrebuildInfo(&toplevel_input, &tl_prebuild);
    REI_ASSERT(tl_prebuild.ResultDataMaxSizeInBytes > 0);
  }
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bl_prebuild = {};
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomlevel_input = {};
  {
    bottomlevel_input.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomlevel_input.Flags
      = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE; // High quality
    bottomlevel_input.NumDescs = geo_descs.size();
    bottomlevel_input.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomlevel_input.pGeometryDescs = geo_descs.data();
    device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomlevel_input, &bl_prebuild);
    REI_ASSERT(bl_prebuild.ResultDataMaxSizeInBytes > 0);
  }

  // Create scratch space (shared by two AS contruction)
  UINT64 scratch_size = max(bl_prebuild.ScratchDataSizeInBytes, tl_prebuild.ScratchDataSizeInBytes);
  scratch_buffer = create_uav_buffer(device, scratch_size);

  // Allocate BLAS and TLAS buffer
  {
    blas_buffer = create_accel_struct_buffer(device, bl_prebuild.ResultDataMaxSizeInBytes);
    tlas_buffer = create_accel_struct_buffer(device, tl_prebuild.ResultDataMaxSizeInBytes);
  }

  // BLAS
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blas_desc {};
  {
    blas_desc.DestAccelerationStructureData = blas_buffer->GetGPUVirtualAddress();
    blas_desc.Inputs = bottomlevel_input;
    blas_desc.SourceAccelerationStructureData = NULL; // used when updating
    blas_desc.ScratchAccelerationStructureData = scratch_buffer->GetGPUVirtualAddress();
  }

  // TLAS
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlas_desc {};
  ComPtr<ID3D12Resource> instance_buffer;
  {
    // TLAS need the instance descs in GPU
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instance_descs {};
    {
      D3D12_RAYTRACING_INSTANCE_DESC desc {};
      FLOAT(&T)[3][4] = desc.Transform;
      T[0][0] = T[1][1] = T[2][2] = 1; // identity matric
      desc.InstanceID = 0;
      desc.InstanceMask = 1;
      desc.InstanceContributionToHitGroupIndex = 0;     // used in shader entry index calculation
      desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE; // not used TODO check this
      desc.AccelerationStructure = blas_buffer->GetGPUVirtualAddress();

      instance_descs.push_back(desc);
    }

    instance_buffer = create_upload_buffer(device, instance_descs.data(), instance_descs.size());
    toplevel_input.InstanceDescs = instance_buffer->GetGPUVirtualAddress();
  }
  {
    tlas_desc.DestAccelerationStructureData = tlas_buffer->GetGPUVirtualAddress();
    tlas_desc.Inputs = toplevel_input;
    tlas_desc.SourceAccelerationStructureData = NULL;
    tlas_desc.ScratchAccelerationStructureData = scratch_buffer->GetGPUVirtualAddress();
  }

  // Build them
  // TODO investigate the postbuild info
  cmd_list->BuildRaytracingAccelerationStructure(&blas_desc, 0, nullptr);
  cmd_list->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::UAV(blas_buffer.Get())); // sync before use
  cmd_list->BuildRaytracingAccelerationStructure(&tlas_desc, 0, nullptr);
  cmd_list->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::UAV(tlas_buffer.Get())); // sync before use

  // FIXME finish uploading before the locally created test buffer out ot scope
  device_resources->flush_command_list();
  device_resources->flush_command_queue_for_frame();
}

void Renderer::build_shader_table() {
  // input
  auto device = device_resources->dxr_device();

  HRESULT hr;

  // prepare
  UINT64 shader_id_bytesize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
  ComPtr<ID3D12StateObjectProperties> state_object_prop;
  hr = device_resources->dxr_pso.As(&state_object_prop);
  REI_ASSERT(SUCCEEDED(hr));

  // Raygen shaders
  {
    struct RaygenRootArgument {
      dxr::RaygenConstantBuffer raygen_cb;
    } raygen_root_arg;
    raygen_root_arg.raygen_cb = {-1, -1, 1, 1};
    UINT64 raygen_root_argument_bytesize = sizeof(RaygenRootArgument);
    void* raygen_shader_id = state_object_prop->GetShaderIdentifier(dxr::c_raygen_shader_name);
    byte* shader_record = new byte[shader_id_bytesize + raygen_root_argument_bytesize];
    memcpy(shader_record, raygen_shader_id, shader_id_bytesize);
    memcpy(shader_record + shader_id_bytesize, &raygen_root_arg, raygen_root_argument_bytesize);
    raygen_shader_table = create_upload_buffer(
      device, shader_id_bytesize + raygen_root_argument_bytesize, shader_record);
    delete[] shader_record;
  }

  // Miss shaders
  { // no arguments
    void* miss_shader_id = state_object_prop->GetShaderIdentifier(dxr::c_miss_shader_name);
    miss_shader_table = create_upload_buffer(device, shader_id_bytesize, miss_shader_id);
  }

  // Hit group
  { // no arguments
    void* hitgroup_shader_id = state_object_prop->GetShaderIdentifier(dxr::c_hit_group_name);
    hitgroup_shader_table = create_upload_buffer(device, shader_id_bytesize, hitgroup_shader_id);
  }
}

} // namespace d3d

} // namespace rei

#endif