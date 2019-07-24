#if DIRECT3D_ENABLED
// Source of d3d_renderer.h
#include "d3d_renderer.h"

#include <array>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_set>

#include <d3d12.h>
#include <d3dx12.h>
#include <windows.h>

#include "../debug.h"
#include "../string_utils.h"

#include "d3d_common_resources.h"
#include "d3d_device_resources.h"
#include "d3d_shader_struct.h"
#include "d3d_utils.h"
#include "d3d_viewport_resources.h"

using std::array;
using std::make_shared;
using std::make_unique;
using std::shared_ptr;
using std::unique_ptr;
using std::unordered_set;
using std::vector;
using std::weak_ptr;
using std::wstring;

namespace rei {

namespace d3d {

// Default Constructor
Renderer::Renderer(HINSTANCE hinstance, Options opt)
    : hinstance(hinstance), is_dxr_enabled(opt.enable_realtime_raytracing) {
  DeviceResources::Options dev_opt;
  dev_opt.is_dxr_enabled = is_dxr_enabled;
  device_resources = std::make_shared<DeviceResources>(hinstance, dev_opt);
}

SwapchainHandle Renderer::create_swapchain(
  SystemWindowID window_id, size_t width, size_t height, size_t rendertarget_count) {
  REI_ASSERT(window_id.platform == SystemWindowID::Win);
  HWND hwnd = window_id.value.hwnd;

  REI_ASSERT(rendertarget_count <= 4);
  FixedVec<std::shared_ptr<DefaultBufferData>, 4> rt_buffers(rendertarget_count);
  for (size_t i = 0; i < rendertarget_count; i++) {
    rt_buffers[i] = make_shared<DefaultBufferData>(this);
  }
  std::shared_ptr<DefaultBufferData> ds_buffer = make_shared<DefaultBufferData>(this);
  auto vp_res = std::make_shared<ViewportResources>(
    device_resources, hwnd, width, height, rt_buffers, ds_buffer);

  auto swapchain = make_shared<SwapchainData>(this);
  swapchain->res = vp_res;
  return SwapchainHandle(swapchain);
}

BufferHandle Renderer::fetch_swapchain_depth_stencil_buffer(SwapchainHandle handle) {
  auto swapchain = to_swapchain(handle);
  return BufferHandle(swapchain->res->m_ds_buffer);
}

BufferHandle Renderer::fetch_swapchain_render_target_buffer(SwapchainHandle handle) {
  auto swapchain = to_swapchain(handle);
  UINT curr_rt_index = swapchain->res->get_current_rt_index();
  return BufferHandle(swapchain->res->m_rt_buffers[curr_rt_index]);
}

BufferHandle Renderer::create_texture_2d(
  size_t width, size_t height, ResourceFormat format, wstring&& name) {
  auto device = device_resources->device();
  DXGI_FORMAT dxgi_format = to_dxgi_format(format);

  auto texture = make_shared<DefaultBufferData>(this);
  {
    BufferDesc meta;
    meta.dimension = ResourceDimension::Texture2D;
    meta.format = format;
    texture->meta = meta;
  }
  {
    ComPtr<ID3D12Resource> tex;
    auto tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(dxgi_format, width, height);
    // TODO add to input config
    tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    auto heap_desc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
    auto init_state = D3D12_RESOURCE_STATE_COMMON;
    HRESULT hr = device->CreateCommittedResource(
      &heap_desc, flags, &tex_desc, init_state, nullptr, IID_PPV_ARGS(&tex));
    REI_ASSERT(SUCCEEDED(hr));
    tex->SetName(name.c_str());
    texture->buffer = tex;
  }

  return BufferHandle(texture);
}

BufferHandle Renderer::create_unordered_access_buffer_2d(
  size_t width, size_t height, ResourceFormat format) {
  auto device = device_resources->device();
  DXGI_FORMAT buffer_format = to_dxgi_format(format);

  DefaultBufferFormat meta;
  meta.dimension = ResourceDimension::Texture2D;
  meta.format = format;

  // Create buffer
  ComPtr<ID3D12Resource> buffer;
  {
    D3D12_RESOURCE_DESC uav_buffer_desc
      = CD3DX12_RESOURCE_DESC::Tex2D(buffer_format, width, height);
    uav_buffer_desc.MipLevels = 1;
    uav_buffer_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    D3D12_HEAP_PROPERTIES heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE;
    D3D12_RESOURCE_STATES init_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    HRESULT hr = device->CreateCommittedResource(
      &heap_prop, heap_flags, &uav_buffer_desc, init_state, nullptr, IID_PPV_ARGS(&buffer));
    REI_ASSERT(SUCCEEDED(hr));
  }

  auto buffer_data = make_shared<DefaultBufferData>(this);
  buffer_data->state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  buffer_data->buffer = buffer;
  buffer_data->meta = meta;
  return BufferHandle(buffer_data);
}

BufferHandle Renderer::create_const_buffer(const ConstBufferLayout& layout, size_t num) {
  auto device = device_resources->device();

  auto buffer = make_unique<UploadBuffer>(*device, get_width(layout), true, num);

  auto buffer_data = make_shared<ConstBufferData>(this);
  buffer_data->state = buffer->init_state();
  buffer_data->buffer = std::move(buffer);
  buffer_data->layout = layout;
  return BufferHandle(buffer_data);
}

void Renderer::update_const_buffer(BufferHandle buffer, size_t index, size_t member, Vec4 value) {
  auto converted = rei_to_D3D(value);
  set_const_buffer(buffer, index, member, &converted, sizeof(converted));
}

void Renderer::update_const_buffer(BufferHandle buffer, size_t index, size_t member, Mat4 value) {
  auto converted = rei_to_D3D(value);
  set_const_buffer(buffer, index, member, &converted, sizeof(converted));
}

ShaderHandle Renderer::create_shader(
  const std::wstring& shader_path, unique_ptr<RasterizationShaderMetaInfo>&& meta) {
  auto shader = make_shared<RasterizationShaderData>(this);

  // compile and retrive root signature
  shader->meta.init(std::move(*meta));
  device_resources->compile_shader(shader_path, shader->compiled_data);
  device_resources->get_root_signature(shader->root_signature, shader->meta);

  if (!(shader->compiled_data.ps_bytecode && shader->compiled_data.vs_bytecode)) {
    REI_ERROR("Shader compilation faild");
    return c_empty_handle;
  }

  return ShaderHandle(shader);
}

ShaderHandle Renderer::create_raytracing_shader(
  const std::wstring& shader_path, unique_ptr<::rei::RaytracingShaderMetaInfo>&& meta) {
  auto shader = make_shared<RaytracingShaderData>(this);

  shader->meta.init(std::move(*meta));
  d3d::RayTracingShaderMetaDesc& d3d_meta = shader->meta;
  device().get_root_signature(d3d_meta.global.desc, shader->root_signature);
  device().get_root_signature(d3d_meta.raygen.desc, shader->raygen.root_signature);
  device().get_root_signature(d3d_meta.hitgroup.desc, shader->hitgroup.root_signature);
  device().get_root_signature(d3d_meta.miss.desc, shader->miss.root_signature);

  build_raytracing_pso(shader_path, *shader, shader->pso);

  // Collect shader IDs
  {
    // prepare
    UINT64 shader_id_bytesize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    ComPtr<ID3D12StateObjectProperties> state_object_prop;
    HRESULT hr = shader->pso.As(&state_object_prop);
    REI_ASSERT(SUCCEEDED(hr));

    auto get_shader_id = [&](const std::wstring& name, const void*& id) {
      id = state_object_prop->GetShaderIdentifier(name.c_str());
    };

    get_shader_id(d3d_meta.hitgroup_name, shader->hitgroup.shader_id);
    get_shader_id(d3d_meta.raygen_name, shader->raygen.shader_id);
    get_shader_id(d3d_meta.miss_name, shader->miss.shader_id);
  }

  return ShaderHandle(shader);
}

ShaderArgumentHandle Renderer::create_shader_argument(
  ShaderHandle shader_h, ShaderArgumentValue arg_value) {
  // TODO carry validation using shader information
  // auto shader = to_shader<ShaderData>(shader_h);

  size_t buf_count = arg_value.total_buffer_count();
  CD3DX12_GPU_DESCRIPTOR_HANDLE base_gpu_descriptor;
  CD3DX12_CPU_DESCRIPTOR_HANDLE base_cpu_descriptor;
  UINT head_index = device_resources->cbv_srv_heap().alloc(
    UINT(buf_count), &base_cpu_descriptor, &base_gpu_descriptor);

  auto device = device_resources->device();
  UINT cnv_srv_descriptor_size = device_resources->cnv_srv_descriptor_size();
  CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_descriptor = base_cpu_descriptor;

  const int cb_count = arg_value.const_buffers.size();
  REI_ASSERT(arg_value.const_buffer_offsets.size() == cb_count);
  for (size_t i = 0; i < cb_count; i++) {
    auto b = to_buffer<ConstBufferData>(arg_value.const_buffers[i]);
    size_t offset = arg_value.const_buffer_offsets[i];
    D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
    desc.BufferLocation = b->buffer->buffer_address(UINT(offset));
    desc.SizeInBytes = b->buffer->effective_bytewidth();
    device->CreateConstantBufferView(&desc, cpu_descriptor);
    cpu_descriptor.Offset(1, cnv_srv_descriptor_size);
  }
  for (auto& h : arg_value.shader_resources) {
    auto b = to_buffer<DefaultBufferData>(h);
    auto meta = b->meta;
    D3D12_SHADER_RESOURCE_VIEW_DESC desc;
    desc.Format = to_dxgi_format_srv(meta.format);
    desc.ViewDimension = to_srv_dimension(meta.dimension);
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    // FIXME handle other cases and options
    bool is_accel_struct = false;
    switch (desc.ViewDimension) {
      case D3D12_SRV_DIMENSION_TEXTURE2D:
        desc.Texture2D.MostDetailedMip = 0;
        desc.Texture2D.MipLevels = 1;
        desc.Texture2D.PlaneSlice = 0;
        desc.Texture2D.ResourceMinLODClamp = 0;
        break;
      case D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE:
        desc.RaytracingAccelerationStructure.Location = b->buffer->GetGPUVirtualAddress();
        is_accel_struct = true;
        break;
      default:
        REI_ERROR("Unhandled case detected");
        break;
    }
    if (is_accel_struct)
      device->CreateShaderResourceView(NULL, &desc, cpu_descriptor);
    else
      device->CreateShaderResourceView(b->buffer.Get(), &desc, cpu_descriptor);
    cpu_descriptor.Offset(1, cnv_srv_descriptor_size);
  }
  for (auto& h : arg_value.unordered_accesses) {
    auto b = to_buffer<DefaultBufferData>(h);
    auto meta = b->meta;
    D3D12_UNORDERED_ACCESS_VIEW_DESC desc {};
    desc.Format = to_dxgi_format(meta.format);
    desc.ViewDimension = to_usv_dimension(meta.dimension);
    switch (desc.ViewDimension) {
      case D3D12_UAV_DIMENSION_TEXTURE2D:
        desc.Texture2D = {};
      default:
        REI_ASSERT("Unhandled case detected");
        break;
    }
    device->CreateUnorderedAccessView(b->buffer.Get(), nullptr, &desc, cpu_descriptor);
    cpu_descriptor.Offset(1, cnv_srv_descriptor_size);
  }

  auto arg_data = make_shared<ShaderArgumentData>(this);
  arg_data->base_descriptor_cpu = base_cpu_descriptor;
  arg_data->base_descriptor_gpu = base_gpu_descriptor;
  // arg_data->cbvs
  arg_data->alloc_index = head_index;
  arg_data->alloc_num = buf_count;
  return ShaderArgumentHandle(arg_data);
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
  /*
  if (!model.get_material()) {
    log("model has no material; use deafult mat");
    model_data->material = default_material;
  } else {
    model_data->material = to_material(model.get_material()->get_graphic_handle());
  }
  */

  // assign a tlas instance id
  model_data->tlas_instance_id = generate_tlas_instance_id();

  model_data->transform = model.get_transform(); // not necessary

  return ModelHandle(model_data);
}

BufferHandle Renderer::create_raytracing_accel_struct(const Scene& scene) {
  // collect geometries and models for AS building
  const auto& scene_models = scene.get_models();
  vector<ModelData> models {};
  models.reserve(scene_models.size());
  for (auto& m : scene_models) {
    ModelData& m_data = *to_model(m->get_rendering_handle());
    GeometryData& g_data = *to_geometry(m->get_geometry()->get_graphic_handle());
    models.push_back(m_data);
  }

  ComPtr<ID3D12Resource> tlas, scratch;
  build_dxr_acceleration_structure(models.data(), models.size(), scratch, tlas);

  DefaultBufferFormat meta;
  meta.dimension = ResourceDimension::AccelerationStructure;
  meta.format = ResourceFormat::AcclerationStructure;

  // NOTE scratch buffer is discared
  auto data = make_shared<DefaultBufferData>(this);
  data->state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  data->buffer = tlas;
  data->meta = meta;
  return BufferHandle(data);
}

BufferHandle Renderer::create_shader_table(const Scene& scene, ShaderHandle raytracing_shader) {
  auto& shader = to_shader<RaytracingShaderData>(raytracing_shader);
  const auto& meta = shader->meta;

  auto tables = make_shared<RaytracingShaderTableData>(this);

  size_t model_count = scene.get_models().size();

  auto device = device_resources->dxr_device();
  auto build = [&](size_t max_root_arguments_width, size_t entry_num, const void* shader_id,
                 shared_ptr<ShaderTable>& shader_table) {
    // Create buffer
    shader_table = make_shared<ShaderTable>(*device, max_root_arguments_width, entry_num);
    // init all shader id
    for (size_t i = 0; i < entry_num; i++) {
      shader_table->update(shader_id, nullptr, 0, i, 0);
    }
  };

  size_t raygen_arg_width = get_root_arguments_size(meta.raygen.desc);
  size_t hitgroup_arg_width = get_root_arguments_size(meta.hitgroup.desc);
  size_t miss_arg_width = get_root_arguments_size(meta.miss.desc);

  build(raygen_arg_width, 1, shader->raygen.shader_id, tables->raygen);
  build(hitgroup_arg_width, model_count, shader->hitgroup.shader_id, tables->hitgroup);
  build(miss_arg_width, model_count, shader->miss.shader_id, tables->miss);
  tables->shader = shader;

  return BufferHandle(tables);
}

void Renderer::update_hitgroup_shader_record(BufferHandle shader_table_h, ModelHandle model_h) {
  auto tables = to_buffer<RaytracingShaderTableData>(shader_table_h);
  auto shader = tables->shader;
  auto model = to_model(model_h);
  auto geometry = std::static_pointer_cast<MeshData>(model->geometry);

  const size_t cnv_srv_descriptor_size = sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
  tables->hitgroup->update(shader->hitgroup.shader_id, &geometry->ind_srv_gpu,
    cnv_srv_descriptor_size, model->tlas_instance_id, 0);
}

void Renderer::begin_render_pass(const RenderPassCommand& cmd) {
  const RenderArea& area = cmd.area;

  auto cmd_list = device_resources->prepare_command_list();

  // Set raster configs
  {
    // NOTE: DirectX viewport starts from top-left to bottom-right.
    D3D12_VIEWPORT d3d_vp {};
    d3d_vp.TopLeftX = 0.0f;
    d3d_vp.TopLeftY = 0.0f;
    d3d_vp.Width = float(area.width);
    d3d_vp.Height = float(area.height);
    d3d_vp.MinDepth = D3D12_MIN_DEPTH;
    d3d_vp.MaxDepth = D3D12_MAX_DEPTH;
    cmd_list->RSSetViewports(1, &d3d_vp);
  }

  {
    D3D12_RECT scissor {};
    scissor.left = 0;
    scissor.top = 0;
    scissor.right = float(area.width);
    scissor.bottom = float(area.height);
    cmd_list->RSSetScissorRects(1, &scissor);
  }

  // hardcode
  const Color clear_color {0, 0, 0, 1};
  const FLOAT clear_depth = 0;
  const FLOAT clear_stentil = 0;

  FixedVec<D3D12_RENDER_PASS_RENDER_TARGET_DESC, 8> rt_descs = {};
  REI_ASSERT(cmd.render_targets.max_size() <= rt_descs.max_size());
  if (cmd.render_targets.size() > 0) {
    for (size_t i = 0; i < cmd.render_targets.size(); i++) {
      BufferHandle render_target_h = cmd.render_targets[i];
      REI_ASSERT(render_target_h != c_empty_handle);
      shared_ptr<DefaultBufferData> rt_texture = to_buffer<DefaultBufferData>(render_target_h);
      REI_ASSERT(rt_texture);

      auto& rt_desc = rt_descs.emplace_back();
      rt_desc.cpuDescriptor = get_rtv_cpu(rt_texture->get_res());
      REI_ASSERT(rt_desc.cpuDescriptor.ptr);
      if (cmd.clear_rt) {
        rt_desc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
        rt_desc.BeginningAccess.Clear.ClearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        fill_color(rt_desc.BeginningAccess.Clear.ClearValue.Color, clear_color);
      } else {
        rt_desc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
      }
      rt_desc.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
    }
  }

  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* ds_desc_ptr = nullptr;
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC ds_desc {};
  if (cmd.depth_stencil != c_empty_handle) {
    shared_ptr<DefaultBufferData> ds_texture = to_buffer<DefaultBufferData>(cmd.depth_stencil);
    REI_ASSERT(ds_texture);
    ds_desc.cpuDescriptor = get_dsv_cpu(ds_texture->get_res());
    REI_ASSERT(ds_desc.cpuDescriptor.ptr);
    if (cmd.clear_ds) {
      ds_desc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
      ds_desc.DepthBeginningAccess.Clear.ClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
      ds_desc.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth = clear_depth;
      ds_desc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
    } else {
      ds_desc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
      ds_desc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
    }
    ds_desc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE; // keep depth ?
    ds_desc.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;

    ds_desc_ptr = &ds_desc;
  }

  D3D12_RENDER_PASS_FLAGS flag = D3D12_RENDER_PASS_FLAG_NONE; // TODO check this what is other enum

  cmd_list->BeginRenderPass(rt_descs.size(), rt_descs.data(), ds_desc_ptr, flag);
}

void Renderer::end_render_pass() {
  auto cmd_list = device_resources->prepare_command_list();
  cmd_list->EndRenderPass();
}

void Renderer::transition(BufferHandle buffer_h, ResourceState state) {
  auto cmd_list = device_resources->prepare_command_list();

  shared_ptr<BufferData> buffer = to_buffer<BufferData>(buffer_h);
  D3D12_RESOURCE_STATES to_state = to_res_state(state);

  if (to_state == buffer->state) { return; }

  auto barr = CD3DX12_RESOURCE_BARRIER::Transition(buffer->get_res(), buffer->state, to_state);
  cmd_list->ResourceBarrier(1, &barr);

  buffer->state = to_state;
}

void Renderer::draw(const DrawCommand& cmd) {
  auto cmd_list = device_resources->prepare_command_list();

  // validate
  REI_ASSERT(cmd.shader);
  shared_ptr<RasterizationShaderData> shader = to_shader<RasterizationShaderData>(cmd.shader);

  // shared setup
  cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // Check root signature
  ID3D12RootSignature* this_root_sign = shader->root_signature.Get();
  cmd_list->SetGraphicsRootSignature(this_root_sign);

  // Check Pipeline state
  ComPtr<ID3D12PipelineState> this_pso;
  device_resources->get_pso(*shader, curr_target_spec, this_pso);
  cmd_list->SetPipelineState(this_pso.Get());

  // Bind shader arguments
  {
    for (UINT i = 0; i < cmd.arguments.size(); i++) {
      if (cmd.arguments[i] == c_empty_handle) continue;
      const auto shader_arg = to_argument(cmd.arguments[i]);
      REI_ASSERT(shader_arg);
      cmd_list->SetGraphicsRootDescriptorTable(i, shader_arg->base_descriptor_gpu);
    }
  }

  // Set geometry buffer and draw
  if (cmd.geo) {
    shared_ptr<MeshData> mesh = to_geometry<MeshData>(cmd.geo);
    cmd_list->IASetVertexBuffers(0, 1, &mesh->vbv);
    cmd_list->IASetIndexBuffer(&mesh->ibv);
    // Draw
    cmd_list->DrawIndexedInstanced(mesh->index_num, 1, 0, 0, 0);
  } else {
    // FIXME better shortcut for blit pass
    cmd_list->IASetVertexBuffers(0, 0, NULL);
    cmd_list->IASetIndexBuffer(NULL);
    cmd_list->DrawInstanced(3, 1, 0, 0);
  }
}

void Renderer::raytrace(ShaderHandle raytrace_shader_h, ShaderArguments arguments,
  BufferHandle shader_table_h, size_t width, size_t height, size_t depth) {
  auto shader = to_shader<RaytracingShaderData>(raytrace_shader_h);
  auto shader_table = to_buffer<RaytracingShaderTableData>(shader_table_h);

  auto cmd_list = device_resources->prepare_command_list_dxr();

  // Change render state
  {
    cmd_list->SetComputeRootSignature(shader->root_signature.Get());
    cmd_list->SetPipelineState1(shader->pso.Get());
  }

  // Bing global resource
  {
    for (UINT i = 0; i < arguments.size(); i++) {
      const auto shader_arg = to_argument(arguments[i]);
      if (shader_arg) {
        cmd_list->SetComputeRootDescriptorTable(i, shader_arg->base_descriptor_gpu);
      }
    }
  }

  // Dispatch
  {
    auto raygen = shader_table->raygen;
    auto hitgroup = shader_table->hitgroup;
    auto miss = shader_table->miss;

    D3D12_DISPATCH_RAYS_DESC desc = {};
    desc.RayGenerationShaderRecord.StartAddress = raygen->buffer_address();
    desc.RayGenerationShaderRecord.SizeInBytes = raygen->effective_bytewidth();
    desc.MissShaderTable.StartAddress = miss->buffer_address();
    desc.MissShaderTable.SizeInBytes = miss->effective_bytewidth();
    desc.MissShaderTable.StrideInBytes = miss->element_bytewidth();
    desc.HitGroupTable.StartAddress = hitgroup->buffer_address();
    desc.HitGroupTable.SizeInBytes = hitgroup->effective_bytewidth();
    desc.HitGroupTable.StrideInBytes = hitgroup->element_bytewidth();
    desc.Width = width;
    desc.Height = height;
    desc.Depth = depth;
    cmd_list->DispatchRays(&desc);
  }
}

void Renderer::copy_texture(BufferHandle src_h, BufferHandle dst_h, bool revert_state) {
  auto device = device_resources->device();
  auto cmd_list = device_resources->prepare_command_list();

  auto src = to_buffer<DefaultBufferData>(src_h);
  auto dst = to_buffer<DefaultBufferData>(dst_h);

  ID3D12Resource* src_resource = src->buffer.Get();
  ID3D12Resource* dst_resource = dst->buffer.Get();

  D3D12_RESOURCE_BARRIER pre_copy[2];
  pre_copy[0] = CD3DX12_RESOURCE_BARRIER::Transition(
    src_resource, src->state, D3D12_RESOURCE_STATE_COPY_SOURCE);
  pre_copy[1] = CD3DX12_RESOURCE_BARRIER::Transition(
    dst_resource, dst->state, D3D12_RESOURCE_STATE_COPY_DEST);
  cmd_list->ResourceBarrier(2, pre_copy);

  cmd_list->CopyResource(dst_resource, src_resource);

  if (revert_state) {
    D3D12_RESOURCE_BARRIER post_copy[2];
    post_copy[0] = CD3DX12_RESOURCE_BARRIER::Transition(
      src_resource, D3D12_RESOURCE_STATE_COPY_SOURCE, src->state);
    post_copy[1] = CD3DX12_RESOURCE_BARRIER::Transition(
      dst_resource, D3D12_RESOURCE_STATE_COPY_DEST, dst->state);
    cmd_list->ResourceBarrier(2, post_copy);
  } else {
    src->state = D3D12_RESOURCE_STATE_COPY_SOURCE;
    dst->state = D3D12_RESOURCE_STATE_COPY_DEST;
  }
}

Renderer* Renderer::prepare() {
  // Finish previous resources commands
  upload_resources();

  // Begin command list recording
  auto cmd_list = device_resources->prepare_command_list();
  cmd_list->SetDescriptorHeaps(1, device_resources->cbv_srv_heap_addr());

  return this;
}

void Renderer::present(SwapchainHandle handle, bool vsync) {
  shared_ptr<SwapchainData> swapchain = to_swapchain(handle);
  auto viewport = swapchain->res;

  // check state
  UINT curr_idx = swapchain->res->get_current_rt_index();
  DefaultBufferData& rt_buffer = *swapchain->res->m_rt_buffers[curr_idx];
  REI_ASSERT(rt_buffer.state == D3D12_RESOURCE_STATE_PRESENT);

  // Done any writing
  device_resources->flush_command_list();

  // Present and flip
  // TODO check state of swapchain buffer
  if (vsync) {
    viewport->swapchain()->Present(1, 0);
  } else {
    viewport->swapchain()->Present(0, 0);
  }
  viewport->flip_backbuffer();

  // Wait
  // FIXME should not wait
  device_resources->flush_command_queue_for_frame();
}

void Renderer::upload_resources() {
  // NOTE: if the cmd_list is closed, we don need to submit it
  if (is_uploading_resources) {
    device_resources->flush_command_list();
    is_uploading_resources = false;
  }
}

void Renderer::build_raytracing_pso(const std::wstring& shader_path,
  const d3d::RaytracingShaderData& shader, ComPtr<ID3D12StateObject>& pso) {
  const d3d::RayTracingShaderMetaDesc& meta = shader.meta;

  // hard-coded properties
  const UINT payload_max_size = sizeof(float) * 4 + sizeof(int) * 1;
  const UINT attr_max_size = sizeof(float) * 2;
  const UINT max_raytracing_recursion_depth
    = min(D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH, 8);

  // short cut
  auto device = device_resources->dxr_device();

  CD3DX12_STATE_OBJECT_DESC raytracing_pipeline_desc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

  ComPtr<IDxcBlob> shader_blobs[3];
  D3D12_SHADER_BYTECODE bytecode[3];
  { // compile and bind the shaders
    const wchar_t* names[3]
      = {meta.raygen_name.c_str(), meta.closest_hit_name.c_str(), meta.miss_name.c_str()};
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
    // not any-hit or intersection shader
    hit_group->SetClosestHitShaderImport(meta.closest_hit_name.c_str());
    hit_group->SetHitGroupExport(meta.hitgroup_name.c_str());
    hit_group->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
  }

  { // DXR shader config
    auto shader_config
      = raytracing_pipeline_desc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    shader_config->Config(payload_max_size, attr_max_size);
  }

  { // bind global root signature
    auto global_root
      = raytracing_pipeline_desc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    global_root->SetRootSignature(shader.root_signature.Get());
  }

  // TODO binding more local root signatures
  { // bind local root signatures and association: hit group
    auto local_root
      = raytracing_pipeline_desc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    local_root->SetRootSignature(shader.hitgroup.root_signature.Get());

    auto rootSignatureAssociation
      = raytracing_pipeline_desc
          .CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    rootSignatureAssociation->SetSubobjectToAssociate(*local_root);
    rootSignatureAssociation->AddExport(meta.hitgroup_name.c_str());
  }

  { // pipeline config
    auto pso_config
      = raytracing_pipeline_desc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    pso_config->Config(max_raytracing_recursion_depth);
  }

  HRESULT hr = device->CreateStateObject(raytracing_pipeline_desc, IID_PPV_ARGS(&pso));
  REI_ASSERT(SUCCEEDED(hr));
}

void Renderer::build_dxr_acceleration_structure(ModelData* models, size_t model_count,
  ComPtr<ID3D12Resource>& scratch_buffer, ComPtr<ID3D12Resource>& tlas_buffer) {
  auto device = device_resources->dxr_device();
  auto cmd_list = device_resources->prepare_command_list_dxr();

  // Prepare: get the prebuild info for TLAS
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tl_prebuild = {};
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS toplevel_input = {};
  toplevel_input.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
  toplevel_input.Flags
    = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE; // High quality
  toplevel_input.NumDescs = model_count;
  toplevel_input.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  toplevel_input.pGeometryDescs = NULL; // instance desc in GPU memory; set latter
  device->GetRaytracingAccelerationStructurePrebuildInfo(&toplevel_input, &tl_prebuild);
  REI_ASSERT(tl_prebuild.ResultDataMaxSizeInBytes > 0);

  // Create scratch space (shared by two AS contruction)
  UINT64 scratch_size = tl_prebuild.ScratchDataSizeInBytes;
  scratch_buffer = create_uav_buffer(device, scratch_size);

  // Allocate BLAS and TLAS buffer
  tlas_buffer = create_accel_struct_buffer(device, tl_prebuild.ResultDataMaxSizeInBytes);

  // TLAS
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlas_desc {};
  ComPtr<ID3D12Resource> instance_buffer;
  {
    // TLAS need the instance descs in GPU
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instance_descs {model_count};
    for (size_t i = 0; i < model_count; i++) {
      ModelData& model = *(models + i);
      MeshData* mesh = to_geometry<MeshData>(model.geometry).get();

      const bool is_opaque = true;

      // TODO pass tranfrom from model
      D3D12_RAYTRACING_INSTANCE_DESC desc {};
      fill_tlas_instance_transform(desc.Transform, model.transform, model_trans_target);
      desc.InstanceID = model.tlas_instance_id;
      desc.InstanceMask = 1;
      desc.InstanceContributionToHitGroupIndex
        = model.tlas_instance_id * 1; // only one entry per instance
      desc.Flags = is_opaque ? D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE
                             : D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE;
      desc.AccelerationStructure = mesh->blas_buffer->GetGPUVirtualAddress();

      instance_descs[i] = desc;
    }

    instance_buffer = create_upload_buffer(device, instance_descs.data(), instance_descs.size());
    toplevel_input.InstanceDescs = instance_buffer->GetGPUVirtualAddress();
  }
  tlas_desc.DestAccelerationStructureData = tlas_buffer->GetGPUVirtualAddress();
  tlas_desc.Inputs = toplevel_input;
  tlas_desc.SourceAccelerationStructureData = NULL;
  tlas_desc.ScratchAccelerationStructureData = scratch_buffer->GetGPUVirtualAddress();

  // Build
  // TODO investigate the postbuild info
  cmd_list->BuildRaytracingAccelerationStructure(&tlas_desc, 0, nullptr);
  cmd_list->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::UAV(tlas_buffer.Get())); // sync before use

  // FIXME finish uploading before the locally created test buffer out ot scope
  // e.g. instance buffer and stractch buffer
  device_resources->flush_command_list();
  device_resources->flush_command_queue_for_frame();
}

void Renderer::set_const_buffer(
  BufferHandle handle, size_t index, size_t member, const void* value, size_t width) {
  shared_ptr<ConstBufferData> buffer = to_buffer<ConstBufferData>(handle);
  // validate
  REI_ASSERT(member < buffer->layout.size());
  REI_ASSERT(width == get_width(buffer->layout[member]));
  // update
  unsigned int local_offset = get_offset(buffer->layout, member);
  buffer->buffer->update(value, width, index, local_offset);
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::get_rtv_cpu(ID3D12Resource* texture) {
  {
    auto* cached = m_rtv_cache.try_get(texture);
    if (cached) return *cached;
  }

  D3D12_CPU_DESCRIPTOR_HANDLE ret;
  device_resources->rtv_heap().alloc(&ret);

  device_resources->device()->CreateRenderTargetView(texture, nullptr, ret);

  m_rtv_cache.insert({texture, ret});
  return ret;
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::get_dsv_cpu(ID3D12Resource* texture) {
  {
    auto* cached = m_dsv_cache.try_get(texture);
    if (cached) return *cached;
  }

  D3D12_CPU_DESCRIPTOR_HANDLE ret;
  device_resources->dsv_heap().alloc(&ret);

  device_resources->device()->CreateDepthStencilView(texture, nullptr, ret);

  m_dsv_cache.insert({texture, ret});
  return ret;
}

} // namespace d3d

} // namespace rei

#endif