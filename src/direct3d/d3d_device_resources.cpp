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
    : hinstance(hinstance), is_dxr_enabled(opt.is_dxr_enabled) {
  HRESULT hr;

  UINT dxgi_factory_flags = 0;
#if DEBUG
  {
    // d3d12 debug layer
    ComPtr<ID3D12Debug> debug_controller;
    REI_ASSERT(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))));
    debug_controller->EnableDebugLayer();
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
  REI_ASSERT(SUCCEEDED(m_device->CreateCommandList(
    node_mask, list_type, m_command_alloc.Get(), init_pip_state, IID_PPV_ARGS(&m_command_list))));
  m_command_list->Close();
  if (is_dxr_enabled) {
    REI_ASSERT(SUCCEEDED(m_command_list->QueryInterface(IID_PPV_ARGS(&m_dxr_command_list))));
  }
  is_using_cmd_list = false;

  // Create fence
  D3D12_FENCE_FLAGS fence_flags = D3D12_FENCE_FLAG_NONE;
  REI_ASSERT(SUCCEEDED(m_device->CreateFence(0, fence_flags, IID_PPV_ARGS(&frame_fence))));
  current_frame_fence = 0;

  // Create buffer-type descriptor heap
  D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
  heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  heap_desc.NumDescriptors = max_descriptor_num;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // for shader resources
  heap_desc.NodeMask = 0;                                      // sinlge GPU
  REI_ASSERT(
    SUCCEEDED(m_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&m_descriotpr_heap))));
  next_descriptor_index = 0;
  m_descriptor_size
    = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DeviceResources::compile_shader(const wstring& shader_path, ShaderCompileResult& result) {
  // routine for bytecode compilation
  auto compile = [&](const string& entrypoint, const string& target) -> ComPtr<ID3DBlob> {
    UINT compile_flags = 0;
#if defined(DEBUG) || !defined(NDEBUG)
    compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    ComPtr<ID3DBlob> return_bytecode;
    ComPtr<ID3DBlob> error_msg;

    D3D_SHADER_MACRO* shader_defines = NULL;
    HRESULT hr = D3DCompileFromFile(shader_path.c_str(), // shader file path
      shader_defines,                                    // preprocessors
      D3D_COMPILE_STANDARD_FILE_INCLUDE,                 // "include file with relative path"
      entrypoint.c_str(),                                // e.g. "VS" or "PS" or "main" or others
      target.c_str(),                                    // "vs_5_0" or "ps_5_0" or similar
      compile_flags,                                     // options
      0,                                                 // more options
      &return_bytecode,                                  // result
      &error_msg                                         // message if error
    );
    if (FAILED(hr)) {
      char* err_str = (char*)error_msg->GetBufferPointer();
      error(err_str);
    }
    return return_bytecode;
  };

  ComPtr<ID3DBlob> vs_bytecode = compile("VS", "vs_5_0");
  ComPtr<ID3DBlob> ps_bytecode = compile("PS", "ps_5_0");


  result = {vs_bytecode, ps_bytecode};

  return;
}

void DeviceResources::create_const_buffers(
  const ShaderData& shader, ShaderConstBuffers& const_buffers) {
  const_buffers.per_frame_CB = make_unique<UploadBuffer<cbPerFrame>>(*m_device.Get(), 1, true);
  UINT64 init_size = 128;
  const_buffers.per_object_CBs
    = make_shared<UploadBuffer<cbPerObject>>(*m_device.Get(), init_size, true);
  const_buffers.next_object_index = 0;
}

void DeviceResources::get_root_signature(ComPtr<ID3D12RootSignature>& root_sign, const ShaderMetaInfo& meta) {
  {
    // TODO return the cached root signature if property
  }

  create_root_signature(meta.root_desc, root_sign);
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

void DeviceResources::get_pso(
  const ShaderData& shader, const RenderTargetSpec& target_spec, ComPtr<ID3D12PipelineState>& pso) {
  // inputs
  const ShaderCompileResult& compiled = shader.compiled_data;
  ComPtr<ID3DBlob> ps_bytecode = compiled.ps_bytecode;
  ComPtr<ID3DBlob> vs_bytecode = compiled.vs_bytecode;
  const ShaderMetaInfo& meta = *shader.meta;
  ComPtr<ID3D12RootSignature> root_sign = shader.root_signature;

  // Try to retried from cache
  const PSOKey cache_key {root_sign.Get(), target_spec};
  auto cached_entry = pso_cache.find(cache_key);
  if (cached_entry != pso_cache.end()) {
    pso = cached_entry->second;
    return;
  }

  // some default value
  UINT rt_num = 1;                            // TODO maybe check this

  // ruotine for creating PSO
  auto create = [&]() -> ComPtr<ID3D12PipelineState> {
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = root_sign.Get();
    desc.VS = {(BYTE*)(vs_bytecode->GetBufferPointer()), vs_bytecode->GetBufferSize()};
    desc.PS = {(BYTE*)(ps_bytecode->GetBufferPointer()), ps_bytecode->GetBufferSize()};
    desc.DS = {};
    desc.HS = {};
    desc.GS = {};
    desc.StreamOutput = {}; // no used
    desc.BlendState = meta.blend_state;
    desc.SampleMask = UINT_MAX; // 0xFFFFFFFF, sample all points if MSAA enabled
    desc.RasterizerState = meta.raster_state;
    desc.DepthStencilState = meta.depth_stencil;
    desc.InputLayout = meta.input_layout;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets = rt_num; // used in the array below
    desc.RTVFormats[0] = target_spec.rt_format;
    desc.DSVFormat = meta.is_depth_stencil_null ? DXGI_FORMAT_UNKNOWN : target_spec.ds_format;
    desc.SampleDesc = target_spec.sample_desc;
    desc.NodeMask = 0; // single GPU
    desc.CachedPSO.pCachedBlob
      = nullptr; // TODO cache in disk; see
                 // https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/Samples/Desktop/D3D12PipelineStateCache
    desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE; // cache
    ComPtr<ID3D12PipelineState> return_pso;
    HRESULT hr = m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&return_pso));
    REI_ASSERT(SUCCEEDED(hr));

    return return_pso;
  };

  pso = create();
  auto inserted = pso_cache.insert({cache_key, pso});
  REI_ASSERT(inserted.second);
}

void DeviceResources::create_mesh_buffer(const Mesh& mesh, MeshData& mesh_res) {
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

  // make a view for vertice buffer, for later use
  D3D12_VERTEX_BUFFER_VIEW vbv;
  vbv.BufferLocation = vert_buffer->GetGPUVirtualAddress();
  vbv.StrideInBytes = sizeof(VertexElement);
  vbv.SizeInBytes = vert_bytesize;

  // Create indices buffer and update data
  ComPtr<ID3D12Resource> ind_buffer = create_default_buffer(device, ind_bytesize);
  ComPtr<ID3D12Resource> ind_upload_buffer
    = upload_to_default_buffer(device, cmd_list, p_indices, ind_bytesize, ind_buffer.Get());

  // make a view for indicew buffer, for later use
  D3D12_INDEX_BUFFER_VIEW ibv;
  ibv.BufferLocation = ind_buffer->GetGPUVirtualAddress();
  ibv.Format = c_index_format;
  ibv.SizeInBytes = ind_bytesize;

  // populate the result
  mesh_res.vert_buffer = vert_buffer;
  mesh_res.vert_upload_buffer = vert_upload_buffer;
  mesh_res.vbv = vbv;
  mesh_res.vertex_num = vertices.size();
  mesh_res.ind_buffer = ind_buffer;
  mesh_res.ind_upload_buffer = ind_upload_buffer;
  mesh_res.ibv = ibv;
  mesh_res.index_num = indices.size();

  if (is_dxr_enabled) {
    // Using srv to allow hit-group shader acessing the geometry attributes
    CD3DX12_GPU_DESCRIPTOR_HANDLE vertex_srv_gpu;
    CD3DX12_CPU_DESCRIPTOR_HANDLE vertex_srv_cpu;
    CD3DX12_GPU_DESCRIPTOR_HANDLE index_srv_gpu;
    CD3DX12_CPU_DESCRIPTOR_HANDLE index_srv_cpu;

    // index first
    auto i_index = this->alloc_descriptor(&index_srv_cpu, &index_srv_gpu);
    auto v_index = this->alloc_descriptor(&vertex_srv_cpu, &vertex_srv_gpu);
    REI_ASSERT(v_index = i_index + 1);

    D3D12_SHADER_RESOURCE_VIEW_DESC common_desc = {};
    common_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    common_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    // vertex srv
    {
      D3D12_SHADER_RESOURCE_VIEW_DESC desc = common_desc;
      desc.Format = DXGI_FORMAT_UNKNOWN;
      desc.Buffer.NumElements = vert_bytesize / sizeof(VertexElement);
      desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
      desc.Buffer.StructureByteStride = vbv.StrideInBytes;
      device->CreateShaderResourceView(vert_buffer.Get(), &desc, vertex_srv_cpu);
    }

    // index src
    {
      // FIXME why the sample do it this way; should be okay to just use a ConstantBuffer<uint>
      D3D12_SHADER_RESOURCE_VIEW_DESC desc = common_desc;
      desc.Format = DXGI_FORMAT_R32_TYPELESS;
      desc.Buffer.NumElements = ind_bytesize / sizeof(int32_t); // pack to R32
      desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
      desc.Buffer.StructureByteStride = 0;
      device->CreateShaderResourceView(ind_buffer.Get(), &desc, index_srv_cpu);
    }

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
      geo_desc.Triangles.VertexBuffer.StrideInBytes = vbv.StrideInBytes;
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
    mesh_res.vert_srv_cpu = vertex_srv_cpu;
    mesh_res.vert_srv_gpu = vertex_srv_gpu;
    mesh_res.vertex_pos_format = c_accel_struct_vertex_pos_format;
    mesh_res.ind_srv_cpu = index_srv_cpu;
    mesh_res.ind_srv_gpu = index_srv_gpu;
    mesh_res.index_format = c_index_format;

    mesh_res.blas_buffer = blas_buffer;
    mesh_res.scratch_buffer = scratch_buffer;
  }
}

ID3D12GraphicsCommandList* DeviceResources::prepare_command_list(ID3D12PipelineState* init_pso) {
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
