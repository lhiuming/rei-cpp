#if DIRECT3D_ENABLED
#include "d3d_device_resources.h"

#if !NDEBUG
// TODO enable d3d12 compiler debug layer
#include <dxgidebug.h>
#endif

#include <d3dx12.h>

#include "../debug.h"

using std::runtime_error;
using std::make_unique;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::vector;
using std::wstring;

namespace rei {

namespace d3d {

DeviceResources::DeviceResources(HINSTANCE h_inst) : hinstance(hinstance) {
  HRESULT hr;

  UINT dxgi_factory_flags = 0;
#if DEBUG
  {
    // d3d12 debug layer
    ComPtr<ID3D12Debug> debug_controller;
    ASSERT(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))));
    debug_controller->EnableDebugLayer();
    // dxgi 4 debug layer
    dxgi_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
  }
#endif

  // DXGI factory
  ASSERT(SUCCEEDED(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory))));

  // D3D device
  IDXGIAdapter* default_adapter = nullptr;
  ASSERT(
    SUCCEEDED(D3D12CreateDevice(default_adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device))));

  // Trace back the adapter
  LUID adapter_liud = device->GetAdapterLuid();
  ASSERT(SUCCEEDED(dxgi_factory->EnumAdapterByLuid(adapter_liud, IID_PPV_ARGS(&dxgi_adapter))));

  // Command list and stuffs
  UINT node_mask = 0; // Single GPU
  D3D12_COMMAND_LIST_TYPE list_type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  D3D12_COMMAND_QUEUE_DESC queue_desc {};
  queue_desc.Type = list_type;
  queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
  queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queue_desc.NodeMask = node_mask;
  ASSERT(SUCCEEDED(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue))));
  ASSERT(SUCCEEDED(device->CreateCommandAllocator(list_type, IID_PPV_ARGS(&command_alloc))));
  ID3D12PipelineState* init_pip_state = nullptr; // no available pip state yet :(
  ASSERT(SUCCEEDED(device->CreateCommandList(
    node_mask, list_type, command_alloc.Get(), init_pip_state, IID_PPV_ARGS(&draw_command_list))));
  draw_command_list->Close();
  ASSERT(SUCCEEDED(device->CreateCommandList(
    node_mask, list_type, command_alloc.Get(), nullptr, IID_PPV_ARGS(&upload_command_list))));
  upload_command_list->Close();
  is_drawing_reset = false;
  is_uploading_reset = false;

  // Create fence
  D3D12_FENCE_FLAGS fence_flags = D3D12_FENCE_FLAG_NONE;
  ASSERT(SUCCEEDED(device->CreateFence(0, fence_flags, IID_PPV_ARGS(&frame_fence))));
  current_frame_fence = 0;

  // Create buffer-type descriptor heap
  D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
  heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  heap_desc.NumDescriptors = max_shading_buffer_view_num;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // for shader resources
  heap_desc.NodeMask = 0;                                      // sinlge GPU
  ASSERT(SUCCEEDED(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&shading_buffer_heap))));
  next_shading_buffer_view_index = 0;
  shading_buffer_view_inc_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

// INIT: compile the default shaders
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

  // FIXME check blow stuffs
  NOT_IMPLEMENTED
  return;

  // Specify the per-frame constant-buffer
  D3D11_BUFFER_DESC cbbd;
  ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC)); // reuse the DESC struct above
  cbbd.Usage = D3D11_USAGE_DEFAULT;
  cbbd.ByteWidth = sizeof(cbPerFrame);
  cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // NOTE: we use Constant Buffer
  cbbd.CPUAccessFlags = 0;
  cbbd.MiscFlags = 0;
  HRESULT hr = d3d11Device->CreateBuffer(&cbbd, NULL, &cbPerFrameBuffer);
  if (FAILED(hr)) throw runtime_error("Per-frame Buufer creation FAILED");

  // Initialize a defautl Light data
  g_light.dir = DirectX::XMFLOAT3(0.25f, 0.5f, 0.0f);
  g_light.ambient = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
  g_light.diffuse = DirectX::XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
}

void DeviceResources::create_const_buffers(const ShaderData& shader, ShaderConstBuffers& const_buffers) {
  const_buffers.per_frame_CB = make_unique<UploadBuffer<cbPerFrame>>(*device.Get(), 1, true);
  UINT64 init_size = 128;
  const_buffers.per_object_CBs = make_shared<UploadBuffer<cbPerObject>>(*device.Get(), init_size, true);
  const_buffers.next_object_index = 0;
}

void DeviceResources::get_root_signature(ComPtr<ID3D12RootSignature>& root_sign) {
  {
    // TODO return the cached root signature if property
  }

  HRESULT hr;

  /*
   * Parameters should be ordered by frequency of update, from most-often updated to least.
   */

  // TODO reflect the shader to get parameters info
  CD3DX12_ROOT_PARAMETER root_par_slots[2];
  root_par_slots[0].InitAsConstantBufferView(0); // per obejct resources
  root_par_slots[1].InitAsConstantBufferView(1); // per frame resources

  UINT par_num = ARRAYSIZE(root_par_slots);
  D3D12_ROOT_PARAMETER* pars = root_par_slots;
  UINT s_sampler_num = 0;               // TODO check this later
  D3D12_STATIC_SAMPLER_DESC* s_sampler_descs = nullptr;
  D3D12_ROOT_SIGNATURE_FLAGS sig_flags
    = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // standard choice
  CD3DX12_ROOT_SIGNATURE_DESC sig_desc(par_num, pars, s_sampler_num, s_sampler_descs, sig_flags);
  ComPtr<ID3DBlob> root_sign_blob;
  ComPtr<ID3DBlob> error_blob;
  hr = D3D12SerializeRootSignature(
    &sig_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_sign_blob, &error_blob);
  if (!SUCCEEDED(hr)) { error("TODO log root signature error here"); }
  UINT node_mask = 0; // single GPU
  hr = device->CreateRootSignature(node_mask, root_sign_blob->GetBufferPointer(),
    root_sign_blob->GetBufferSize(), IID_PPV_ARGS(&root_sign));
  ASSERT(SUCCEEDED(hr));
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
    HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&return_pso));
    ASSERT(SUCCEEDED(hr));

    return return_pso;
  };

  pso = create();
  auto inserted = pso_cache.insert({cache_key, pso});
  ASSERT(inserted.second);
}

// common routine for debug
void DeviceResources::create_mesh_buffer_common(
  const vector<VertexElement>& vertices, const vector<std::uint16_t>& indices, MeshData& mesh_res) {
  ComPtr<ID3D12GraphicsCommandList> cmd_list = upload_command_list;
  if (!is_uploading_reset) {
    // first reset in this frame
    upload_command_list->Reset(command_alloc.Get(), nullptr);
    is_uploading_reset = true;
  }

  // shared routine for create a default buffer in GPU
  auto create = [&](UINT bytesize) -> ComPtr<ID3D12Resource> {
    D3D12_HEAP_PROPERTIES heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bytesize);
    D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE;
    D3D12_RESOURCE_STATES init_state = D3D12_RESOURCE_STATE_COMMON;
    D3D12_CLEAR_VALUE* p_clear_value = nullptr; // optional
    ComPtr<ID3D12Resource> result;
    HRESULT hr = device->CreateCommittedResource(
      &heap_prop, heap_flags, &desc, init_state, p_clear_value, IID_PPV_ARGS(&result));
    ASSERT(SUCCEEDED(hr));
    return result;
  };

  // shared routine for upload data to default heap, with an intermediate upload buffer (newly
  // created)
  auto upload = [&](const void* data, UINT64 bsize,
                  ComPtr<ID3D12Resource> dest_buffer) -> ComPtr<ID3D12Resource> {
    // create upalod buffer
    D3D12_HEAP_PROPERTIES heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bsize);
    D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE;
    D3D12_RESOURCE_STATES init_state = D3D12_RESOURCE_STATE_GENERIC_READ;
    D3D12_CLEAR_VALUE* p_clear_value = nullptr; // optional
    ComPtr<ID3D12Resource> upload_buffer;
    HRESULT hr = device->CreateCommittedResource(
      &heap_prop, heap_flags, &desc, init_state, p_clear_value, IID_PPV_ARGS(&upload_buffer));
    ASSERT(SUCCEEDED(hr));

    // pre-upload transition
    D3D12_RESOURCE_BARRIER pre_upload = CD3DX12_RESOURCE_BARRIER::Transition(
      dest_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    cmd_list->ResourceBarrier(1, &pre_upload);

    // upload it
    UINT upload_offset = 0;
    UINT upload_first_sub_res = 0;
    D3D12_SUBRESOURCE_DATA sub_res_data = {};
    sub_res_data.pData = data;     // memory for uploading from
    sub_res_data.RowPitch = bsize; // length of memory to copy
    sub_res_data.SlicePitch
      = sub_res_data.SlicePitch; // for buffer-type sub-res, same with row pitch  TODO check this
    UpdateSubresources(cmd_list.Get(), dest_buffer.Get(), upload_buffer.Get(), upload_offset,
      upload_first_sub_res, 1, &sub_res_data);

    // post-upload transition
    D3D12_RESOURCE_BARRIER post_upload = CD3DX12_RESOURCE_BARRIER::Transition(
      dest_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    cmd_list->ResourceBarrier(1, &post_upload);

    return upload_buffer;
  };

  const void* p_vertices = &(vertices[0]);
  UINT64 vert_bytesize = vertices.size() * sizeof(vertices[0]);
  const void* p_indices = &(indices[0]);
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
  ibv.Format = DXGI_FORMAT_D32_FLOAT;
  ibv.Format = DXGI_FORMAT_R16_UINT;
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
  // Allocate constant buffer for this model
  
}

void DeviceResources::create_mesh_buffer(const Mesh& mesh, MeshData& mesh_res) {
  // Collect the source data
  // TODO pool this vectors
  vector<VertexElement> vertices;
  vector<std::uint16_t> indices;
  Vec3 default_normal {1.0, 1.0, 1.0};
  for (const auto& v : mesh.get_vertices())
    vertices.emplace_back(v.coord, v.color, default_normal);
  for (const auto& t : mesh.get_triangles()) {
    indices.push_back(t.a);
    indices.push_back(t.b);
    indices.push_back(t.c);
  }

  create_mesh_buffer_common(vertices, indices, mesh_res);
}


void DeviceResources::create_debug_mesh_buffer(MeshData& mesh_res) {
  // the data for the cube
  vector<VertexElement> v = {
    //     Position               Color                     Normal
    VertexElement(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f),
    VertexElement(-1.0f, +1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, +1.0f, -1.0f),
    VertexElement(+1.0f, +1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, +1.0f, +1.0f, -1.0f),
    VertexElement(+1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, +1.0f, -1.0f, -1.0f),
    VertexElement(-1.0f, -1.0f, +1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, +1.0f),
    VertexElement(-1.0f, +1.0f, +1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, +1.0f, +1.0f),
    VertexElement(+1.0f, +1.0f, +1.0f, 1.0f, 1.0f, 1.0f, 1.0f, +1.0f, +1.0f, +1.0f),
    VertexElement(+1.0f, -1.0f, +1.0f, 1.0f, 1.0f, 1.0f, 1.0f, +1.0f, -1.0f, +1.0f),
  };
  vector<std::uint16_t> indices = {// front face
    0, 1, 2, 0, 2, 3,

    // back face
    4, 6, 5, 4, 7, 6,

    // left face
    4, 5, 1, 4, 1, 0,

    // right face
    3, 2, 6, 3, 6, 7,

    // top face
    1, 5, 6, 1, 6, 2,

    // bottom face
    4, 0, 3, 4, 3, 7};

  create_mesh_buffer_common(v, indices, mesh_res);
  return;

  /*
  HRESULT hr;

  // Create the vertex buffer data object
  D3D11_BUFFER_DESC vertexBufferDesc;
  ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
  vertexBufferDesc.Usage
    = D3D11_USAGE_DEFAULT; // how the buffer will be read from and written to; use default
  vertexBufferDesc.ByteWidth = sizeof(VertexElement) * ARRAYSIZE(v); // size of the buffer
  vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;             // used as vertex buffer
  vertexBufferDesc.CPUAccessFlags = 0;                               //  we don't use it
  vertexBufferDesc.MiscFlags = 0;                                    // extra flags; not using
  vertexBufferDesc.StructureByteStride = NULL;                       // not using
  D3D11_SUBRESOURCE_DATA vertexBufferData;                           // parameter struct ?
  ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
  vertexBufferData.pSysMem = v;
  hr = d3d11Device->CreateBuffer(&vertexBufferDesc, // buffer description
    &vertexBufferData,                              // parameter set above
    &(this->cubeVertBuffer)                         // receive the returned ID3D11Buffer object
  );
  if (FAILED(hr)) throw runtime_error("Create cube vertex buffer FAILED");

  // Create the index buffer data object
  D3D11_BUFFER_DESC indexBufferDesc;
  ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
  indexBufferDesc.Usage = D3D11_USAGE_DEFAULT; // I guess this is for DRAM type
  indexBufferDesc.ByteWidth = sizeof(DWORD) * ARRAYSIZE(indices);
  indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER; // index !
  indexBufferDesc.CPUAccessFlags = 0;
  indexBufferDesc.MiscFlags = 0;
  indexBufferDesc.StructureByteStride = NULL;
  D3D11_SUBRESOURCE_DATA indexBufferData;
  ZeroMemory(&indexBufferData, sizeof(indexBufferData));
  indexBufferData.pSysMem = indices;
  hr = d3d11Device->CreateBuffer(&indexBufferDesc, &indexBufferData, &(this->cubeIndexBuffer));
  if (FAILED(hr)) throw runtime_error("Create cube index buffer FAILED");

  // Create a constant-buffer for the cube
  D3D11_BUFFER_DESC cbbd;
  ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
  cbbd.Usage = D3D11_USAGE_DEFAULT;
  cbbd.ByteWidth = sizeof(cbPerObject);
  cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // NOTE: we use Constant Buffer
  hr = d3d11Device->CreateBuffer(&cbbd, NULL, &cubeConstBuffer);
  if (FAILED(hr)) throw runtime_error("Cube Const Buffer creation FAILED");
  */
}

void DeviceResources::flush_command_queue_for_frame() {
  // Advance frame fence value
  current_frame_fence++;

  // Wait to finish all submitted commands (and reach current frame fence value)
  command_queue->Signal(frame_fence.Get(), current_frame_fence);
  if (frame_fence->GetCompletedValue() < current_frame_fence) {
    // TODO can we reuse the event handle
    LPSECURITY_ATTRIBUTES default_security = nullptr;
    LPCSTR evt_name = nullptr;
    DWORD flags = 0; // dont signal init state; auto reset;
    DWORD access_mask = EVENT_ALL_ACCESS;
    HANDLE evt_handle = CreateEventEx(default_security, evt_name, flags, access_mask);

    HRESULT hr = frame_fence->SetEventOnCompletion(current_frame_fence, evt_handle);
    ASSERT(SUCCEEDED(hr));

    WaitForSingleObject(evt_handle, INFINITE);
    CloseHandle(evt_handle);
  }
}

} // namespace d3d

} // namespace rei

#endif
