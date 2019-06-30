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

  // Get input layout
  // TODO shader reflection
  std::vector<D3D12_INPUT_ELEMENT_DESC> vertex_ele_descs
    = {{
         "POSITION", 0,                  // a Name and an Index to map elements in the shader
         DXGI_FORMAT_R32G32B32A32_FLOAT, // enum member of DXGI_FORMAT; define the format of the
                                         // element
         0, // input slot; kind of a flexible and optional configuration
         0, // byte offset
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // ADVANCED, discussed later; about instancing
         0                                           // ADVANCED; also for instancing
       },
      {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
        sizeof(VertexElement::pos), // skip the first 3 coordinate data
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
        sizeof(VertexElement::pos)
          + sizeof(VertexElement::color), // skip the fisrt 3 coordinnate and 4 colors ata
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

  result = {vs_bytecode, ps_bytecode, vertex_ele_descs};

  // TODO add refelction for input layout and const buffers

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

void DeviceResources::get_root_signature(ComPtr<ID3D12RootSignature>& root_sign) {
  {
    // TODO return the cached root signature if property
  }

  /*
   * Parameters should be ordered by frequency of update, from most-often updated to least.
   */

  // TODO reflect the shader to get parameters info
  CD3DX12_ROOT_PARAMETER root_par_slots[2];
  root_par_slots[0].InitAsConstantBufferView(0); // per obejct resources
  root_par_slots[1].InitAsConstantBufferView(1); // per frame resources

  UINT par_num = ARRAYSIZE(root_par_slots);
  D3D12_ROOT_PARAMETER* pars = root_par_slots;
  UINT s_sampler_num = 0; // TODO check this later
  D3D12_STATIC_SAMPLER_DESC* s_sampler_descs = nullptr;
  D3D12_ROOT_SIGNATURE_FLAGS sig_flags
    = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // standard choice
  CD3DX12_ROOT_SIGNATURE_DESC root_desc(par_num, pars, s_sampler_num, s_sampler_descs, sig_flags);

  create_root_signature(root_desc, root_sign);
}

void DeviceResources::create_root_signature(
  const D3D12_ROOT_SIGNATURE_DESC& root_desc, ComPtr<ID3D12RootSignature>& root_sign) {
  HRESULT hr;
  ComPtr<ID3DBlob> root_sign_blob;
  ComPtr<ID3DBlob> error_blob;
  hr = D3D12SerializeRootSignature(
    &root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_sign_blob, &error_blob);
  if (!SUCCEEDED(hr)) { error("TODO log root signature error here"); }
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
  vector<D3D12_INPUT_ELEMENT_DESC> vertex_input_descs = compiled.vertex_input_descs;
  ComPtr<ID3D12RootSignature> root_sign = shader.root_signature;

  // Try to retried from cache
  const PSOKey cache_key {root_sign.Get(), target_spec};
  auto cached_entry = pso_cache.find(cache_key);
  if (cached_entry != pso_cache.end()) {
    pso = cached_entry->second;
    return;
  }

  // some default value
  D3D12_BLEND_DESC blend_state = CD3DX12_BLEND_DESC(D3D12_DEFAULT);            // TODO check this
  D3D12_RASTERIZER_DESC raster_state = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // TODO check this
  raster_state.FrontCounterClockwise = true; // d3d default is false
  D3D12_DEPTH_STENCIL_DESC depth_stencil
    = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // TODO check this
  D3D12_PRIMITIVE_TOPOLOGY_TYPE primitive_topo
    = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // TODO maybe check this
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
    desc.BlendState = blend_state;
    desc.SampleMask = UINT_MAX; // 0xFFFFFFFF, sample all points if MSAA enabled
    desc.RasterizerState = raster_state;
    desc.DepthStencilState = depth_stencil;
    desc.InputLayout.pInputElementDescs = vertex_input_descs.data();
    desc.InputLayout.NumElements = vertex_input_descs.size();
    desc.PrimitiveTopologyType = primitive_topo;
    desc.NumRenderTargets = rt_num; // used in the array below
    desc.RTVFormats[0] = target_spec.rt_format;
    desc.DSVFormat = target_spec.ds_format;
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

// common routine for debug
void DeviceResources::create_mesh_buffer_common(
  const vector<VertexElement>& vertices, const vector<std::uint16_t>& indices, MeshData& mesh_res) {
  if (REI_ERRORIF(vertices.size() == 0) || REI_ERRORIF(indices.size() == 0)) {
    return;
  }

  ID3D12Device* device = this->device();
  ID3D12GraphicsCommandList* cmd_list = this->prepare_command_list();

  // shared routine for create a default buffer in GPU
  auto create = [&](UINT bytesize) -> ComPtr<ID3D12Resource> {
    return create_default_buffer(device, bytesize);
  };

  // shared routine for upload data to default heap
  auto upload = [&](const void* data, UINT64 bsize,
                  ComPtr<ID3D12Resource> dest_buffer) -> ComPtr<ID3D12Resource> {
    return upload_to_default_buffer(device, cmd_list, data, bsize, dest_buffer.Get());
  };

  const void* p_vertices = vertices.data();
  UINT64 vert_bytesize = vertices.size() * sizeof(vertices[0]);
  const void* p_indices = indices.data();
  UINT64 ind_bytesize = indices.size() * sizeof(indices[0]);

  // Create vertices buffer and upload data
  ComPtr<ID3D12Resource> vert_buffer = create(vert_bytesize);
  ComPtr<ID3D12Resource> vert_upload_buffer = upload(p_vertices, vert_bytesize, vert_buffer);

  // make a view for vertice buffer, for later use
  D3D12_VERTEX_BUFFER_VIEW vbv;
  vbv.BufferLocation = vert_buffer->GetGPUVirtualAddress();
  vbv.StrideInBytes = sizeof(VertexElement);
  vbv.SizeInBytes = vert_bytesize;

  // Create indices buffer and update data
  ComPtr<ID3D12Resource> ind_buffer = create(ind_bytesize);
  ComPtr<ID3D12Resource> ind_upload_buffer = upload(p_indices, ind_bytesize, ind_buffer);

  // make a view for indicew buffer, for later use
  D3D12_INDEX_BUFFER_VIEW ibv;
  ibv.BufferLocation = ind_buffer->GetGPUVirtualAddress();
  ibv.Format = c_index_format;
  ibv.SizeInBytes = ind_bytesize;

  // populate the result
  mesh_res.vert_buffer = vert_buffer;
  mesh_res.vert_upload_buffer = vert_upload_buffer;
  mesh_res.vbv = vbv;
  mesh_res.ind_buffer = ind_buffer;
  mesh_res.ind_upload_buffer = ind_upload_buffer;
  mesh_res.ibv = ibv;
  mesh_res.index_num = indices.size();
}

void DeviceResources::create_model_buffer(const Model& model, ModelData& model_data) {
  // Allocate constant buffer view descriptor for this model
  // TODO not needed, we are using root parameter
}

void DeviceResources::create_mesh_buffer(const Mesh& mesh, MeshData& mesh_res) {
  // Collect the source data
  // TODO pool this vectors
  vector<VertexElement> vertices;
  vector<std::uint16_t> indices;
  Vec3 default_normal {1.0, 1.0, 1.0};
  for (const auto& v : mesh.get_vertices())
    vertices.emplace_back(v.coord, v.color, v.normal);
  for (const auto& t : mesh.get_triangles()) {
    indices.push_back(t.a);
    indices.push_back(t.b);
    indices.push_back(t.c);
  }

  create_mesh_buffer_common(vertices, indices, mesh_res);
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
