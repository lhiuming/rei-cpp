#if DIRECT3D_ENABLED
#include "d3d_device_resources.h"

#if !NDEBUG
// TODO enable d3d12 compiler debug layer
#include <dxgidebug.h>
#endif

#include <d3dx12.h>

#include "../debug.h"
#include "d3d_utils.h"

using std::make_shared;
using std::make_unique;
using std::runtime_error;
using std::shared_ptr;
using std::string;
using std::vector;
using std::wstring;

namespace rei {

namespace d3d {

DeviceResources::DeviceResources(HINSTANCE h_inst, Options opt)
    : hinstance(h_inst), is_dxr_enabled(opt.is_dxr_enabled) {
  HRESULT hr;

  UINT dxgi_factory_flags = 0;
#if DEBUG
  {
    // d3d12 debug layer
    // ComPtr<ID3D12Debug> debug_controller;
    ComPtr<ID3D12Debug1> debug_controller;
    REI_ASSERT(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))));
    debug_controller->EnableDebugLayer();
    // deeper debug, might be alot slower
    // debug_controller->SetEnableGPUBasedValidation(true);
    // dxgi 4 debug layer
    dxgi_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
  }
#endif

  // DXGI factory
  REI_ASSERT(SUCCEEDED(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&m_dxgi_factory))));

  // D3D device
  IDXGIAdapter* default_adapter = nullptr;
  REI_ASSERT(
    SUCCEEDED(D3D12CreateDevice(default_adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device))));
  if (is_dxr_enabled) {
    REI_ASSERT(SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&m_dxr_device))));
  }

  // Trace back the adapter
  LUID adapter_liud = m_device->GetAdapterLuid();
  REI_ASSERT(
    SUCCEEDED(m_dxgi_factory->EnumAdapterByLuid(adapter_liud, IID_PPV_ARGS(&m_dxgi_adapter))));

  // Command list and stuffs
  UINT node_mask = 0; // Single GPU
  D3D12_COMMAND_LIST_TYPE list_type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  D3D12_COMMAND_QUEUE_DESC queue_desc {};
  queue_desc.Type = list_type;
  queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
  queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queue_desc.NodeMask = node_mask;
  REI_ASSERT(SUCCEEDED(m_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_command_queue))));
  REI_ASSERT(
    SUCCEEDED(m_device->CreateCommandAllocator(list_type, IID_PPV_ARGS(&m_command_alloc))));
  ID3D12PipelineState* init_pip_state = nullptr; // no available pip state yet :(
  REI_ASSERT(SUCCEEDED(m_device->CreateCommandList(node_mask, list_type, m_command_alloc.Get(),
    init_pip_state, IID_PPV_ARGS(&m_command_list_legacy))));
  REI_ASSERT(SUCCEEDED(m_command_list_legacy->QueryInterface(IID_PPV_ARGS(&m_command_list))));
  m_command_list->Close();
  is_using_cmd_list = false;

  // Create fence
  D3D12_FENCE_FLAGS fence_flags = D3D12_FENCE_FLAG_NONE;
  REI_ASSERT(SUCCEEDED(m_device->CreateFence(0, fence_flags, IID_PPV_ARGS(&frame_fence))));
  current_frame_fence = 0;

  // Create  descriptor heaps
  m_cbv_srv_heap = NaiveDescriptorHeap(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 256, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
  m_cbv_srv_shader_nonvisible_heap= NaiveDescriptorHeap(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 64);
  m_rtv_heap = NaiveDescriptorHeap(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 8);
  m_dsv_heap = NaiveDescriptorHeap(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 8);
}

void DeviceResources::get_root_signature(
  const D3D12_ROOT_SIGNATURE_DESC& root_desc, ComPtr<ID3D12RootSignature>& root_sign) {
  /*
   * TODO maybe cache the root signatures
   * NOTE: d3d12 seems already do caching root signature for identical DESC
   */
  create_root_signature(root_desc, root_sign);
}

void DeviceResources::create_root_signature(
  const D3D12_ROOT_SIGNATURE_DESC& root_desc, ComPtr<ID3D12RootSignature>& root_sign) {
  HRESULT hr;
  ComPtr<ID3DBlob> root_sign_blob;
  ComPtr<ID3DBlob> error_blob;
  hr = D3D12SerializeRootSignature(
    &root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_sign_blob, &error_blob);
  if (!SUCCEEDED(hr)) {
    if (error_blob != nullptr) {
      const char* msg = (char*)error_blob->GetBufferPointer();
      REI_ERROR(msg);
    } else {
      REI_ERROR("Root Signature creation fail with not error message");
    }
    root_sign = nullptr;
    return;
  }
  UINT node_mask = 0; // single GPU
  hr = m_device->CreateRootSignature(node_mask, root_sign_blob->GetBufferPointer(),
    root_sign_blob->GetBufferSize(), IID_PPV_ARGS(&root_sign));
  REI_ASSERT(SUCCEEDED(hr));
}

void DeviceResources::create_mesh_buffer(const Mesh& mesh, MeshUploadResult& res) {
  // Collect the source data
  // TODO pool this vectors
  vector<VertexElement> vertices;
  vector<std::uint16_t> indices;
  for (const auto& v : mesh.get_vertices())
    vertices.emplace_back(v.coord, v.color, v.normal);
  for (const auto& t : mesh.get_triangles()) {
    indices.push_back(t.a);
    indices.push_back(t.b);
    indices.push_back(t.c);
  }

  ID3D12Device* device = this->device();
  ID3D12GraphicsCommandList* cmd_list = this->prepare_command_list();

  const void* p_vertices = vertices.data();
  const UINT64 vert_bytesize = vertices.size() * sizeof(vertices[0]);
  const void* p_indices = indices.data();
  const UINT64 ind_bytesize = indices.size() * sizeof(indices[0]);

  // Create vertices buffer and upload data
  ComPtr<ID3D12Resource> vert_buffer = create_default_buffer(device, vert_bytesize);
  ComPtr<ID3D12Resource> vert_upload_buffer
    = upload_to_default_buffer(device, cmd_list, p_vertices, vert_bytesize, vert_buffer.Get());

  // Create indices buffer and update data
  ComPtr<ID3D12Resource> ind_buffer = create_default_buffer(device, ind_bytesize);
  ComPtr<ID3D12Resource> ind_upload_buffer
    = upload_to_default_buffer(device, cmd_list, p_indices, ind_bytesize, ind_buffer.Get());

  // populate the result
  res.vert_buffer = vert_buffer;
  res.vert_upload_buffer = vert_upload_buffer;
  res.vertex_num = vertices.size();

  res.ind_buffer = ind_buffer;
  res.ind_upload_buffer = ind_upload_buffer;
  res.index_num = indices.size();

  if (is_dxr_enabled) {
    D3D12_SHADER_RESOURCE_VIEW_DESC common_desc = {};
    common_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    common_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    // Build BLAS for this mesh
    ComPtr<ID3D12Resource> blas_buffer;
    ComPtr<ID3D12Resource> scratch_buffer; // TODO release this a proper timming, or pool this
    {
      ID3D12Device5* dxr_device = this->dxr_device();
      ID3D12GraphicsCommandList4* dxr_cmd_list = this->prepare_command_list_dxr();

      const bool is_opaque = true;

      D3D12_RAYTRACING_GEOMETRY_DESC geo_desc {};
      geo_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
      geo_desc.Triangles.Transform3x4 = NULL; // not used TODO check this
      geo_desc.Triangles.IndexFormat = c_index_format;
      geo_desc.Triangles.VertexFormat = c_accel_struct_vertex_pos_format;
      geo_desc.Triangles.IndexCount = indices.size();
      geo_desc.Triangles.VertexCount = vertices.size();
      geo_desc.Triangles.IndexBuffer = ind_buffer->GetGPUVirtualAddress();
      geo_desc.Triangles.VertexBuffer.StartAddress = vert_buffer->GetGPUVirtualAddress();
      geo_desc.Triangles.VertexBuffer.StrideInBytes = sizeof(VertexElement);
      if (is_opaque) geo_desc.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

      const UINT c_mesh_count = 1;

      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bl_prebuild = {};
      D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomlevel_input = {};
      bottomlevel_input.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
      bottomlevel_input.Flags
        = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE; // High quality
      bottomlevel_input.NumDescs = c_mesh_count;
      bottomlevel_input.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
      bottomlevel_input.pGeometryDescs = &geo_desc;
      dxr_device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomlevel_input, &bl_prebuild);
      REI_ASSERT(bl_prebuild.ResultDataMaxSizeInBytes > 0);

      // Create scratch space
      UINT64 scratch_size = bl_prebuild.ScratchDataSizeInBytes;
      scratch_buffer = create_uav_buffer(dxr_device, scratch_size);

      // Allocate BLAS buffer
      blas_buffer = create_accel_struct_buffer(dxr_device, bl_prebuild.ResultDataMaxSizeInBytes);

      // BLAS desc
      D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blas_desc {};
      blas_desc.DestAccelerationStructureData = blas_buffer->GetGPUVirtualAddress();
      blas_desc.Inputs = bottomlevel_input;
      blas_desc.SourceAccelerationStructureData = NULL; // used when updating
      blas_desc.ScratchAccelerationStructureData = scratch_buffer->GetGPUVirtualAddress();

      // Build
      // TODO investigate the postbuild info
      dxr_cmd_list->BuildRaytracingAccelerationStructure(&blas_desc, 0, nullptr);
      dxr_cmd_list->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::UAV(blas_buffer.Get())); // sync before use
    }

    // populate the result
    res.blas_buffer = blas_buffer;
    res.scratch_buffer = scratch_buffer;
  }
}

ID3D12GraphicsCommandList4* DeviceResources::prepare_command_list(ID3D12PipelineState* init_pso) {
  if (!is_using_cmd_list) {
    HRESULT hr = m_command_list->Reset(m_command_alloc.Get(), nullptr);
    REI_ASSERT((SUCCEEDED(hr)));
    is_using_cmd_list = true;
  }
  return m_command_list.Get();
}

void DeviceResources::flush_command_list() {
  // TODO check is not empty
  HRESULT hr = m_command_list->Close();
  REI_ASSERT(SUCCEEDED(hr));
  is_using_cmd_list = false;
  ID3D12CommandList* temp_cmd_lists[1] = {m_command_list.Get()};
  m_command_queue->ExecuteCommandLists(1, temp_cmd_lists);
}

void DeviceResources::flush_command_queue_for_frame() {
  HRESULT hr;

  // Advance frame fence value
  current_frame_fence++;

  // Wait to finish all submitted commands (and reach current frame fence value)
  m_command_queue->Signal(frame_fence.Get(), current_frame_fence);
  if (frame_fence->GetCompletedValue() < current_frame_fence) {
    // TODO can we reuse the event handle
    LPSECURITY_ATTRIBUTES default_security = nullptr;
    LPCSTR evt_name = nullptr;
    DWORD flags = 0; // dont signal init state; auto reset;
    DWORD access_mask = EVENT_ALL_ACCESS;
    HANDLE evt_handle = CreateEventEx(default_security, evt_name, flags, access_mask);
    REI_ASSERT(evt_handle);

    if (evt_handle == 0) return;

    hr = frame_fence->SetEventOnCompletion(current_frame_fence, evt_handle);
    REI_ASSERT(SUCCEEDED(hr));

    WaitForSingleObject(evt_handle, INFINITE);
    CloseHandle(evt_handle);
  }

  // Reset allocator since commands are finished
  hr = m_command_alloc->Reset();
  REI_ASSERT(SUCCEEDED(hr));
}

} // namespace d3d

} // namespace rei

#endif
