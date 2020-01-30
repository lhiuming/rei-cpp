// Source of renderer.h
#include "renderer.h"

#include <array>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_set>

#include <d3d12.h>
#include <d3dx12.h>
#include <windows.h>

#include "debug.h"
#include "string_utils.h"

#include "direct3d/d3d_common_resources.h"
#include "direct3d/d3d_device_resources.h"
#include "direct3d/d3d_shader_struct.h"
#include "direct3d/d3d_utils.h"
#include "direct3d/d3d_renderer_resources.h"

using std::array;
using std::make_shared;
using std::make_unique;
using std::move;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::unordered_set;
using std::vector;
using std::weak_ptr;
using std::wstring;

namespace rei {

using namespace d3d;

// Default rt buffer name for convinience
constexpr static array<const wchar_t*, 8> c_rt_buffer_names {
  L"Render Target [0]",
  L"Render Target [1]",
  L"Render Target [2]",
  L"Render Target [3]",
  L"Render Target [4]",
  L"Render Target [5]",
  L"Render Target [6]",
  L"Render Target [7]",
};

// Convertions utils
template <unsigned int N>
FixedVec<D3D_SHADER_MACRO, N + 1> to_hlsl_macros(
  const FixedVec<ShaderCompileConfig::Macro, N>& definitions) {
  FixedVec<D3D_SHADER_MACRO, N + 1> hlsl_macros {};
  for (auto& d : definitions) {
    D3D_SHADER_MACRO m = {d.name.c_str(), d.definition.c_str()};
    hlsl_macros.push_back(m);
  }
  hlsl_macros.push_back({NULL, NULL});
  return hlsl_macros;
}

// Default Constructor
Renderer::Renderer(HINSTANCE hinstance, Options opt)
    : hinstance(hinstance), is_dxr_enabled(opt.enable_realtime_raytracing) {
  DeviceResources::Options dev_opt;
  dev_opt.is_dxr_enabled = is_dxr_enabled;
  device_resources = std::make_shared<DeviceResources>(hinstance, dev_opt);
}

SwapchainHandle Renderer::create_swapchain(
  SystemWindowID window_id, size_t width, size_t height, size_t rendertarget_count) {
  REI_ASSERT(window_id.platform == SystemWindowID::Platform::Win);
  HWND hwnd = window_id.value.hwnd;

  auto device = device_resources->device();

  // Create the Swapchain object
  ComPtr<IDXGISwapChain3> swapchain_obj;
  {
    /*
     * NOTE: In DX12, IDXGIDevice is not available anymore, so IDXGIFactory/IDXGIAdaptor has to be
     * created/specified, rather than being found through IDXGIDevice.
     */

    auto dxgi_factory = device_resources->dxgi_factory();
    auto cmd_queue = device_resources->command_queue();

    HRESULT hr;

    ComPtr<IDXGISwapChain1> base_swapchain_obj;
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc = {}; // use it if you need fullscreen
    fullscreen_desc.RefreshRate.Numerator = 60;
    fullscreen_desc.RefreshRate.Denominator = 1;
    fullscreen_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    fullscreen_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    DXGI_SWAP_CHAIN_DESC1 chain_desc = {};
    chain_desc.Width = width;
    chain_desc.Height = height;
    chain_desc.Format = curr_target_spec.dxgi_rt_format;
    chain_desc.Stereo = FALSE;
    chain_desc.SampleDesc = curr_target_spec.sample_desc;
    chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    chain_desc.BufferCount = rendertarget_count; // total frame buffer count
    chain_desc.Scaling = DXGI_SCALING_STRETCH;
    chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // TODO DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
    chain_desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    chain_desc.Flags = 0;
    hr = dxgi_factory->CreateSwapChainForHwnd(cmd_queue, hwnd, &chain_desc,
      NULL, // windowed app
      NULL, // optional
      &base_swapchain_obj);
    REI_ASSERT(SUCCEEDED(hr));

    hr = base_swapchain_obj.As(&swapchain_obj);
    REI_ASSERT(SUCCEEDED(hr));
  }

  // Create handle for each render target
  FixedVec<BufferHandle, 8> render_target_handles;
  {
    D3D12_RENDER_TARGET_VIEW_DESC* default_rtv_desc = nullptr; // default initialization
    for (UINT i = 0; i < rendertarget_count; i++) {
      ComPtr<ID3D12Resource> rt_buffer;
      HRESULT hr = swapchain_obj->GetBuffer(i, IID_PPV_ARGS(&rt_buffer));
      REI_ASSERT(SUCCEEDED(hr));
      debug_name(rt_buffer, c_rt_buffer_names[i]);

      TextureBuffer render_target;
      render_target.buffer = rt_buffer;
      render_target.dimension = ResourceDimension::Texture2D;
      auto rt_data = make_shared<BufferData>(this);
      rt_data->res = std::move(render_target);
      rt_data->state = D3D12_RESOURCE_STATE_PRESENT; // TODO is this alway correct ?
      render_target_handles.emplace_back(std::move(rt_data));
    }
  }

  auto swapchain = make_shared<SwapchainData>(this);
  swapchain->curr_rt_index = 0;
  swapchain->res_object = std::move(swapchain_obj);
  swapchain->render_targets = std::move(render_target_handles);
  return SwapchainHandle(swapchain);
}

BufferHandle Renderer::fetch_swapchain_render_target_buffer(SwapchainHandle handle) {
  auto swapchain = to_swapchain(handle);
  UINT curr_rt_index = swapchain->curr_rt_index;
  return swapchain->render_targets[curr_rt_index];
}

BufferHandle Renderer::create_texture(
  const TextureDesc& desc, ResourceState init_state, const Name& name) {
  auto device = device_resources->device();
  DXGI_FORMAT dxgi_format = to_dxgi_format(desc.format);
  D3D12_RESOURCE_STATES d3d_init_state = to_res_state(init_state);

  TextureBuffer texture;
  texture.dimension = ResourceDimension::Texture2D; // TODO may be more in future
  texture.format = desc.format;
  {
    ComPtr<ID3D12Resource> tex;
    auto res_desc = CD3DX12_RESOURCE_DESC::Tex2D(dxgi_format, UINT(desc.width), UINT(desc.height));
    // TODO add to input config
    if (desc.flags.allow_depth_stencil) res_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    if (desc.flags.allow_render_target) res_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if (desc.flags.allow_unordered_access)
      res_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    auto heap_desc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto heap_flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
    D3D12_CLEAR_VALUE _optimized_clear = {}; // special clear value; usefull for framebuffer types
    D3D12_CLEAR_VALUE* optimized_clear_ptr = NULL;
    if (desc.flags.allow_depth_stencil) {
      _optimized_clear.Format = dxgi_format;
      _optimized_clear.DepthStencil = curr_target_spec.ds_clear;
      optimized_clear_ptr = &_optimized_clear;
    }
    HRESULT hr = device->CreateCommittedResource(
      &heap_desc, heap_flags, &res_desc, d3d_init_state, optimized_clear_ptr, IID_PPV_ARGS(&tex));
    REI_ASSERT(SUCCEEDED(hr));
    debug_name(tex, name);
    texture.buffer = tex;
  }
#if DEBUG
  texture.name = name;
#endif

  auto res_data = make_shared<BufferData>(this);
  res_data->res = std::move(texture);
  res_data->state = d3d_init_state;
  return BufferHandle(res_data);
}

void Renderer::upload_texture(BufferHandle handle, const void* pixels) {
  REI_ASSERT(handle != c_empty_handle);
  REI_ASSERT(pixels);
  auto buffer = to_buffer(handle);
  REI_ASSERT(buffer->res.holds<TextureBuffer>());
  TextureBuffer& texture = buffer->res.get<TextureBuffer>();

  ID3D12Device* device = device_resources->device();
  ID3D12GraphicsCommandList* cmd_list = device_resources->prepare_command_list();

  auto desc = texture.buffer->GetDesc();
  const UINT64 width = desc.Width;
  const UINT64 height = desc.Height;
  const UINT stride = get_bytesize(desc.Format);
  const UINT upload_pitch = (width * stride + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
  const UINT upload_size = height * upload_pitch;
  ComPtr<ID3D12Resource> upload_buffer;
  {
    D3D12_RESOURCE_DESC desc {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = upload_size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES props {};
    props.Type = D3D12_HEAP_TYPE_UPLOAD;
    props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    HRESULT hr = device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
      D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&upload_buffer));

    REI_ASSERT(SUCCEEDED(hr));

    void* mapped;
    upload_buffer->Map(0, nullptr, &mapped);
    for (int y = 0; y < height; y++)
      memcpy((void*)((uintptr_t)mapped + y * upload_pitch), (const byte*)pixels + y * width * stride, width * stride);
    upload_buffer->Unmap(0, nullptr);

    debug_name(upload_buffer, L"Texture Upload");
  }

  if ((buffer->state & D3D12_RESOURCE_STATE_COPY_DEST) != D3D12_RESOURCE_STATE_COPY_DEST) {
    this->transition(handle, ResourceState::CopyDestination);
  }

  D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
  srcLocation.pResource = upload_buffer.Get();
  srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  srcLocation.PlacedFootprint.Footprint.Format = desc.Format;
  srcLocation.PlacedFootprint.Footprint.Width = width;
  srcLocation.PlacedFootprint.Footprint.Height = height;
  srcLocation.PlacedFootprint.Footprint.Depth = 1;
  srcLocation.PlacedFootprint.Footprint.RowPitch = upload_pitch;
  D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
  dstLocation.pResource = texture.buffer.Get();
  dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  dstLocation.SubresourceIndex = 0;
  cmd_list->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, NULL);

  m_delayed_release_raw.push_back(upload_buffer);

  buffer->state = D3D12_RESOURCE_STATE_COPY_DEST;
}

BufferHandle Renderer::create_const_buffer(
  const ConstBufferLayout& layout, size_t num, const Name& name) {
  auto device = device_resources->device();

  ConstBuffer cb;
  cb.buffer = make_unique<UploadBuffer>(*device, get_width(layout), true, num);
  cb.buffer->resource()->SetName(name.c_str());
  cb.layout = layout;
  auto buffer_data = make_shared<BufferData>(this);
  buffer_data->state = cb.buffer->init_state();
  buffer_data->res = std::move(cb);
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

ShaderHandle Renderer::create_shader(const std::wstring& shader_path,
  RasterizationShaderMetaInfo&& meta, const ShaderCompileConfig& config) {
  auto shader = make_shared<RasterizationShaderData>(this);
  shader->meta.init(std::move(meta));

  // compile and retrive root signature
  ComPtr<ID3DBlob> vs_bytecode;
  ComPtr<ID3DBlob> ps_bytecode;
  {
    auto shader_defines = to_hlsl_macros(config.definitions);

    // routine for bytecode compilation
    // TODO use dxcompiler instead
    auto compile = [&](const string& entrypoint, const string& target) -> ComPtr<ID3DBlob> {
      UINT compile_flags = 0;
#if defined(DEBUG)
      compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
      ComPtr<ID3DBlob> return_bytecode;
      ComPtr<ID3DBlob> error_msg;

      HRESULT hr = D3DCompileFromFile(shader_path.c_str(), // shader file path
        shader_defines.data(),                             // preprocessors
        D3D_COMPILE_STANDARD_FILE_INCLUDE,                 // "include file with relative path"
        entrypoint.c_str(),                                // e.g. "VS" or "PS" or "main" or others
        target.c_str(),                                    // "vs_5_0" or "ps_5_0" or similar
        compile_flags,                                     // options
        0,                                                 // more options
        &return_bytecode,                                  // result
        &error_msg                                         // message if error
      );
      if (FAILED(hr)) {
        if (error_msg) {
          char* err_str = (char*)error_msg->GetBufferPointer();
          error(err_str);
        } else {
          REI_ERROR("Shader Compiler failed with no error msg");
        }
      }
      return return_bytecode;
    };

    vs_bytecode = compile("VS", "vs_5_1");
    ps_bytecode = compile("PS", "ps_5_1");
  }

  return finalize_shader_creation(vs_bytecode, ps_bytecode, shader);
}

ShaderHandle Renderer::create_shader(const char* vertex_shader, const char* pixel_shader,
  RasterizationShaderMetaInfo&& meta, const ShaderCompileConfig& config) {
  auto shader = make_shared<RasterizationShaderData>(this);
  shader->meta.init(std::move(meta));

  ComPtr<ID3DBlob> vs_bytecode = compile_shader(nullptr, vertex_shader, "VS", "vs_5_1");
  ComPtr<ID3DBlob> ps_bytecode = compile_shader(nullptr, pixel_shader, "PS", "ps_5_1");

  return finalize_shader_creation(vs_bytecode, ps_bytecode, shader);
}

ShaderHandle Renderer::finalize_shader_creation(ComPtr<ID3DBlob> vs_bytecode,
  ComPtr<ID3DBlob> ps_bytecode, std::shared_ptr<RasterizationShaderData> shader) {
  if (!(ps_bytecode && vs_bytecode)) {
    REI_ERROR("Shader compilation faild");
    return c_empty_handle;
  }

  // Retrive/Create root signature
  // TODO allow create pso here
  device_resources->get_root_signature(shader->meta.root_signature.desc, shader->root_signature);
  REI_ASSERT(shader->root_signature);
  {
    ID3D12RootSignature* root_sign = shader->root_signature.Get();
    RasterShaderDesc& meta = shader->meta;
    ID3D12Device* device = device_resources->device();

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
    desc.pRootSignature = root_sign;
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
    desc.InputLayout = meta.input_layout.get_layout_desc();
    desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets = meta.get_rtv_formats(desc.RTVFormats);
    desc.DSVFormat = meta.get_dsv_format();
    desc.SampleDesc = DXGI_SAMPLE_DESC {1, 0};
    desc.NodeMask = 0; // single GPU
    desc.CachedPSO.pCachedBlob
      = nullptr; // TODO cache in disk; see
                 // https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/Samples/Desktop/D3D12PipelineStateCache
    desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE; // cache
    HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&shader->pso));
    REI_ASSERT(SUCCEEDED(hr));
  }

  return ShaderHandle(shader);
}

ShaderHandle Renderer::create_shader(const std::wstring& shader_path, ComputeShaderMetaInfo&& meta,
  const ShaderCompileConfig& config) {
  auto shader = make_shared<ComputeShaderData>(this);

  shader->meta.init(move(meta));

  // compile
  auto shader_macros = to_hlsl_macros(config.definitions);
  ComPtr<ID3DBlob> compiled
    = compile_shader(shader_path.c_str(), "CS", "cs_5_1", shader_macros.data());

  // get root-signature
  device_resources->get_root_signature(shader->meta.signature.desc, shader->root_signature);
  REI_ASSERT(shader->root_signature);

  // crate pso
  {
    ID3D12RootSignature* root_sign = shader->root_signature.Get();
    ComputeShaderDesc& meta = shader->meta;
    ID3D12Device* device = device_resources->device();

    D3D12_COMPUTE_PIPELINE_STATE_DESC desc;
    desc.pRootSignature = root_sign;
    desc.CS = {(BYTE*)(compiled->GetBufferPointer()), compiled->GetBufferSize()};
    desc.NodeMask = 0;                           // single GPU
    desc.CachedPSO = {NULL, 0};                  // no caching currently
    desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE; // TODO add debug option
    HRESULT hr = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&shader->pso));
    REI_ASSERT(SUCCEEDED(hr));
  }

  return ShaderHandle(shader);
}

ShaderHandle Renderer::create_shader(const std::wstring& shader_path,
  RaytracingShaderMetaInfo&& meta, const ShaderCompileConfig& config) {
  auto shader = make_shared<RaytracingShaderData>(this);

  shader->meta.init(std::move(meta));
  RayTraceShaderDesc& d3d_meta = shader->meta;
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

ShaderArgumentHandle Renderer::create_shader_argument(ShaderArgumentValue arg_value) {
  size_t buf_count = arg_value.total_buffer_count();
  CD3DX12_GPU_DESCRIPTOR_HANDLE base_gpu_descriptor;
  CD3DX12_CPU_DESCRIPTOR_HANDLE base_cpu_descriptor;
  UINT head_index = device_resources->cbv_srv_heap().alloc(
    UINT(buf_count), &base_cpu_descriptor, &base_gpu_descriptor);

  auto device = device_resources->device();
  UINT cnv_srv_descriptor_size = device_resources->cnv_srv_descriptor_size();
  CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_descriptor = base_cpu_descriptor;

  // Constsant Buffer Views
  const int cb_count = arg_value.const_buffers.size();
  REI_ASSERT(arg_value.const_buffer_offsets.size() == cb_count);
  for (size_t i = 0; i < cb_count; i++) {
    auto buffer = to_buffer(arg_value.const_buffers[i]);
    size_t offset = arg_value.const_buffer_offsets[i];
    REI_ASSERT(buffer);
    REI_ASSERT(buffer->res.holds<ConstBuffer>());
    ConstBuffer& cb = buffer->res.get<ConstBuffer>();
    D3D12_CONSTANT_BUFFER_VIEW_DESC desc {};
    desc.BufferLocation = cb.buffer->buffer_address(UINT(offset));
    desc.SizeInBytes = cb.buffer->element_bytewidth();
    device->CreateConstantBufferView(&desc, cpu_descriptor);
    cpu_descriptor.Offset(1, cnv_srv_descriptor_size);
  }

  // Shader Resource Views
  for (auto& h : arg_value.shader_resources) {
    auto buffer = to_buffer(h);
    ID3D12Resource* create_ptr;
    D3D12_SHADER_RESOURCE_VIEW_DESC desc {};
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    buffer->res.match( // TODO handle other cases
      [](auto& arg) { REI_NOT_IMPLEMENTED },
      [&](IndexBuffer& ind) {
        create_ptr = ind.res.ptr.Get();
        // Raw/typeless buffer view
        desc.Format = DXGI_FORMAT_R32_TYPELESS; // TODO really?
        desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        desc.Buffer.FirstElement = 0;
        desc.Buffer.NumElements = ind.res.bytesize / sizeof(int32_t); // pack as R32 typeless
        desc.Buffer.StructureByteStride = 0;                          // Typeless?
        desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
      },
      [&](VertexBuffer& vert) {
        create_ptr = vert.res.ptr.Get();
        desc.Format = DXGI_FORMAT_UNKNOWN; // TODO really?
        desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        desc.Buffer.FirstElement = 0;
        desc.Buffer.NumElements = vert.vertex_count;
        desc.Buffer.StructureByteStride = vert.stride;
        desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
      },
      [&](TextureBuffer& tex) {
        create_ptr = tex.buffer.Get();
        desc.Format = to_dxgi_format_srv(tex.format);
        desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MostDetailedMip = 0;
        desc.Texture2D.MipLevels = 1;
        desc.Texture2D.PlaneSlice = 0;
        desc.Texture2D.ResourceMinLODClamp = 0;
      },
      [&](TlasBuffer& tlas) {
        create_ptr = NULL;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        desc.RaytracingAccelerationStructure.Location = tlas.buffer->GetGPUVirtualAddress();
      });
    device->CreateShaderResourceView(create_ptr, &desc, cpu_descriptor);
    cpu_descriptor.Offset(1, cnv_srv_descriptor_size);
  }

  // Unordered Access Views
  for (auto& h : arg_value.unordered_accesses) {
    auto buffer = to_buffer(h);
    REI_ASSERT(buffer);
    ID3D12Resource* create_ptr;
    D3D12_UNORDERED_ACCESS_VIEW_DESC desc {};
    buffer->res.match( // TODO handle other cases
      [](auto& arg) { REI_NOT_IMPLEMENTED },
      [&](TextureBuffer& tex) {
        create_ptr = tex.buffer.Get();
        desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        desc.Texture2D = {};
      });
    device->CreateUnorderedAccessView(create_ptr, nullptr, &desc, cpu_descriptor);
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

GeometryBufferHandles Renderer::create_geometry(const LowLevelGeometryDesc& geometry, const Name& name) {
  ID3D12GraphicsCommandList* cmd_list = device_resources->prepare_command_list();
  ID3D12Device* device = device_resources->device();

  auto upload = [&](ID3D12Resource* dest_buffer, ID3D12Resource* upload_buffer, const void* data,
                  UINT64 bytesize) {
    // pre-upload transition
    D3D12_RESOURCE_BARRIER pre_upload = CD3DX12_RESOURCE_BARRIER::Transition(
      dest_buffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    cmd_list->ResourceBarrier(1, &pre_upload);

    // upload it using the upload-buffer as an intermediate space
    UINT upload_offset = 0;
    UINT upload_first_sub_res = 0;
    D3D12_SUBRESOURCE_DATA sub_res_data = {};
    sub_res_data.pData = data;        // memory for uploading from
    sub_res_data.RowPitch = bytesize; // length of memory to copy
    sub_res_data.SlicePitch
      = sub_res_data.SlicePitch; // for buffer-type sub-res, same with row pitch  TODO check this
    UpdateSubresources(
      cmd_list, dest_buffer, upload_buffer, upload_offset, upload_first_sub_res, 1, &sub_res_data);

    // post-upload transition
    D3D12_RESOURCE_BARRIER post_upload = CD3DX12_RESOURCE_BARRIER::Transition(
      dest_buffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    cmd_list->ResourceBarrier(1, &post_upload);
  };

  auto create = [&](UINT64 bsize, const void* init_data, bool dynamic, bool uploadable) {
    ComPtr<ID3D12Resource> ret_buffer;
    if (uploadable) {
      ret_buffer = create_upload_buffer(device, bsize, init_data);
    } else {
      ret_buffer = create_default_buffer(device, bsize);
      ComPtr<ID3D12Resource> upload_buffer = create_upload_buffer(device, bsize);
      upload(ret_buffer.Get(), upload_buffer.Get(), init_data, bsize);
      m_delayed_release_raw.emplace_back(upload_buffer);
      debug_name(upload_buffer, L"Geometry Upload");
    }
    return CommittedResource(ret_buffer, bsize, uploadable);
  };

  // both dynamic in content and dynamic in size
  const bool is_uploadable = geometry.flags.dynamic;
  const bool is_dynamic = geometry.flags.dynamic;

  const UINT64 index_bsize = geometry.index.element_bytesize * geometry.index.element_count;
  const UINT64 vertex_bsize = geometry.vertex.element_bytesize * geometry.vertex.element_count;

  GeometryBufferHandles ret {};
  ID3D12Resource* i_buffer;
  {
    IndexBuffer ib;
    ib.res = create(index_bsize, geometry.index.addr, is_dynamic, is_uploadable);
    i_buffer = ib.res.ptr.Get();
    ib.format = c_index_format;
    ib.index_count = geometry.index.element_count;
    auto data = new_buffer();
    data->res = move(ib);
    data->state = D3D12_RESOURCE_STATE_INDEX_BUFFER;
    ret.index_buffer = std::move(data);
  }
  ID3D12Resource* v_buffer;
  {
    VertexBuffer vb;
    vb.res = create(vertex_bsize, geometry.vertex.addr, is_dynamic, is_uploadable);
    v_buffer = vb.res.ptr.Get();
    vb.vertex_count = geometry.vertex.element_count;
    vb.stride = geometry.vertex.element_bytesize;
    auto data = new_buffer();
    data->res = move(vb);
    data->state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    ret.vertex_buffer = std::move(data);
  }
  debug_name(i_buffer, name);
  debug_name(v_buffer, name);
  if (geometry.flags.include_blas) {
    D3D12_SHADER_RESOURCE_VIEW_DESC common_desc = {};
    common_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    common_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    // Build BLAS for this mesh
    ComPtr<ID3D12Resource> blas_buffer;
    ComPtr<ID3D12Resource> scratch_buffer;
    {
      ID3D12Device5* dxr_device = device_resources->dxr_device();
      ID3D12GraphicsCommandList4* dxr_cmd_list = device_resources->prepare_command_list_dxr();

      const bool is_opaque = true;

      D3D12_RAYTRACING_GEOMETRY_DESC geo_desc {};
      geo_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
      geo_desc.Triangles.Transform3x4 = NULL; // not used TODO check this
      geo_desc.Triangles.IndexFormat = c_index_format;
      geo_desc.Triangles.VertexFormat = c_accel_struct_vertex_pos_format;
      geo_desc.Triangles.IndexCount = geometry.index.element_count;
      geo_desc.Triangles.VertexCount = geometry.vertex.element_count;
      geo_desc.Triangles.IndexBuffer = i_buffer->GetGPUVirtualAddress();
      geo_desc.Triangles.VertexBuffer.StartAddress = v_buffer->GetGPUVirtualAddress();
      geo_desc.Triangles.VertexBuffer.StrideInBytes = geometry.vertex.element_bytesize;
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

    m_delayed_release_raw.push_back(scratch_buffer);
    debug_name(scratch_buffer, L"BLAS scratch");
    debug_name(blas_buffer, name);

    BlasBuffer blas;
    blas.res = CommittedResource(blas_buffer, -1, false);
    auto data = new_buffer();
    data->res = move(blas);
    ret.blas_buffer = move(data);
  }
  return ret;
}

GeometryBufferHandles Renderer::create_geometry(const GeometryDesc& desc) {
  auto& mesh = dynamic_cast<const Mesh&>(*(desc.geometry));
  using index_t = uint16_t;
  using vertex_t = VertexElement;
  const size_t i_count = mesh.get_triangles().size() * 3;
  const size_t v_count = mesh.get_vertices().size();
  vector<index_t> indicies;
  indicies.reserve(i_count);
  vector<vertex_t> vertices;
  vertices.reserve(v_count);
  for (auto t : mesh.get_triangles()) {
    indicies.push_back(t.a);
    indicies.push_back(t.b);
    indicies.push_back(t.c);
  }
  for (auto v : mesh.get_vertices()) {
    vertices.emplace_back(VertexElement(v.coord, v.color, v.normal));
  }
  LowLevelGeometryDesc ll_desc {};
  ll_desc.index = {indicies.data(), i_count, sizeof(index_t)};
  ll_desc.vertex = {vertices.data(), v_count, sizeof(vertex_t)};
  ll_desc.flags = desc.flags;
  return create_geometry(ll_desc, desc.geometry->name());
}

void Renderer::update_geometry(
  BufferHandle handle, LowLevelGeometryData data, size_t dest_element_offset) {
  auto buffer = to_buffer(handle);

  ID3D12Device* device = device_resources->device();

  UINT min_count = data.element_count + dest_element_offset;
  UINT64 required_bytesize = data.element_bytesize * min_count;

  auto update = [&](CommittedResource& res) {
    if (!res.is_system_memory) {
      REI_ERROR("Updating GPU-local bufer is illegal!");
      return;
    }

    if (res.bytesize < required_bytesize) {
      res.ptr = create_upload_buffer(device, required_bytesize);
      res.bytesize = required_bytesize;
    }

    buffer->state == D3D12_RESOURCE_STATE_GENERIC_READ;

    // Write data if required
    if (data.addr) {
      UINT64 upload_bytesize = data.element_count * data.element_bytesize;
      byte* sys_mem;
      res.ptr->Map(0, nullptr, (void**)&sys_mem);
      memcpy(sys_mem + dest_element_offset * data.element_bytesize, data.addr, upload_bytesize);
      res.ptr->Unmap(0, nullptr);
    }
  };

  buffer->res.match(                                   //
    [](auto& a) { REI_ERROR("Invalid Buffer Type"); }, //
    [&](IndexBuffer& ib) {
      update(ib.res);
      ib.index_count = (std::max)(ib.index_count, min_count);
    },
    [&](VertexBuffer& vb) {
      update(vb.res);
      vb.vertex_count = (std::max)(vb.vertex_count, min_count);
    });
}

BufferHandle Renderer::create_raytracing_accel_struct(const RaytraceSceneDesc& desc, const Name& name) {
  const size_t model_count = desc.blas_buffer.size();

  // Collect geometries and models for AS building
  vector<ID3D12Resource*> blas;
  blas.reserve(model_count);
  for (auto& handle : desc.blas_buffer) {
    auto buffer = to_buffer(handle);
    REI_ASSERT(buffer->res.holds<BlasBuffer>());
    blas.push_back(buffer->get_res());
  }

  // Create
  TlasBuffer tlas;
  {
    BuildAccelStruct scene;
    scene.instance_count = model_count;
    scene.blas = blas.data();
    scene.instance_id = desc.instance_id.data();
    scene.transform = desc.transform.data();
    build_dxr_acceleration_structure(std::move(scene), tlas.buffer);
  }
  debug_name(tlas.buffer, name);

  auto data = new_buffer();
  data->res = move(tlas);
  data->state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  return BufferHandle(data);
}

BufferHandle Renderer::create_shader_table(size_t intersec_count, ShaderHandle raytracing_shader) {
  auto& shader = to_shader<RaytracingShaderData>(raytracing_shader);
  const auto& meta = shader->meta;

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

  ShaderTableBuffer tables;
  tables.shader = shader;
  build(raygen_arg_width, 1, shader->raygen.shader_id, tables.raygen);
  build(hitgroup_arg_width, intersec_count, shader->hitgroup.shader_id, tables.hitgroup);
  build(miss_arg_width, intersec_count, shader->miss.shader_id, tables.miss);

  auto data = new_buffer();
  data->res = move(tables);
  data->state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS; // FIXME really?
  return BufferHandle(data);
}

BufferHandle Renderer::create_shader_table(const Scene& scene, ShaderHandle raytracing_shader) {
  return create_shader_table(scene.get_models().size(), raytracing_shader);
}

void Renderer::update_shader_table(const UpdateShaderTable& cmd) {
  auto shader = to_shader<RaytracingShaderData>(cmd.shader);
  auto buffer = to_buffer(cmd.shader_table);
  REI_ASSERT(buffer);
  REI_ASSERT(buffer->res.holds<ShaderTableBuffer>());
  ShaderTableBuffer& tables = buffer->res.get<ShaderTableBuffer>();

  shared_ptr<ShaderTable> table;
  const void* shader_id;
  switch (cmd.table_type) {
    case UpdateShaderTable::TableType::Hitgroup:
      table = tables.hitgroup;
      shader_id = shader->hitgroup.shader_id;
      break;
    default:
      REI_ERROR("Unhandled shader table type");
      break;
  }

  FixedVec<D3D12_GPU_DESCRIPTOR_HANDLE, 8> local_args {};
  for (ShaderArgumentHandle h : cmd.arguments) {
    auto arg = to_argument(h);
    if (arg) { local_args.push_back(arg->base_descriptor_gpu); }
  }

  const size_t gpu_descriptor_size = sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
  table->update(
    shader_id, local_args.data(), local_args.size() * gpu_descriptor_size, cmd.index, 0);
}

void Renderer::begin_render_pass(const RenderPassCommand& cmd) {
  const RenderViewaport viewport = cmd.viewport;
  const RenderArea& area = cmd.area;

  auto cmd_list = device_resources->prepare_command_list();

  // Set raster configs
  {
    // NOTE: DirectX viewport starts from top-left to bottom-right.
    D3D12_VIEWPORT d3d_vp {};
    d3d_vp.TopLeftX = viewport.offset_left;
    d3d_vp.TopLeftY = viewport.offset_top;
    d3d_vp.Width = viewport.width;
    d3d_vp.Height = viewport.height;
    d3d_vp.MinDepth = D3D12_MIN_DEPTH;
    d3d_vp.MaxDepth = D3D12_MAX_DEPTH;
    cmd_list->RSSetViewports(1, &d3d_vp);
  }

  if (!area.is_empty()) {
    D3D12_RECT scissor {};
    scissor.left = float(area.offset_left);
    scissor.top = float(area.offset_top);
    scissor.right = float(area.offset_left + area.width);
    scissor.bottom = float(area.offset_top + area.height);
    cmd_list->RSSetScissorRects(1, &scissor);
  }

  // hardcode
  const Color clear_color {0, 0, 0, 1};
  const FLOAT clear_depth = 0;
  const FLOAT clear_stentil = 0;

  FixedVec<D3D12_RENDER_PASS_RENDER_TARGET_DESC, 8> rt_descs = {};
  REI_ASSERT(cmd.render_targets.max_size <= rt_descs.max_size);
  if (cmd.render_targets.size() > 0) {
    for (size_t i = 0; i < cmd.render_targets.size(); i++) {
      BufferHandle render_target_h = cmd.render_targets[i];
      REI_ASSERT(render_target_h != c_empty_handle);
      auto rt_buffer = to_buffer(render_target_h);
      REI_ASSERT(rt_buffer);
      REI_ASSERT(rt_buffer->res.holds<TextureBuffer>());
      auto& tex = rt_buffer->res.get<TextureBuffer>();

      auto& rt_desc = rt_descs.emplace_back();
      rt_desc.cpuDescriptor = get_rtv_cpu(tex.buffer.Get());
      REI_ASSERT(rt_desc.cpuDescriptor.ptr);
      if (cmd.clear_rt) {
        rt_desc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
        rt_desc.BeginningAccess.Clear.ClearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        fill_color(rt_desc.BeginningAccess.Clear.ClearValue.Color, clear_color);
      } else {
        rt_desc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
      }
      rt_desc.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
    }
  }

  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* ds_desc_ptr = nullptr;
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC ds_desc {};
  if (cmd.depth_stencil != c_empty_handle) {
    auto ds_buffer = to_buffer(cmd.depth_stencil);
    REI_ASSERT(ds_buffer && ds_buffer->res.holds<TextureBuffer>());
    auto& tex = ds_buffer->res.get<TextureBuffer>();
    ds_desc.cpuDescriptor = get_dsv_cpu(tex.buffer.Get());
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

  auto buffer = to_buffer(buffer_h);
  D3D12_RESOURCE_STATES to_state = to_res_state(state);

  if (to_state == buffer->state) { return; }

  auto barr = CD3DX12_RESOURCE_BARRIER::Transition(buffer->get_res(), buffer->state, to_state);
  cmd_list->ResourceBarrier(1, &barr);

  buffer->state = to_state;
}

void Renderer::barrier(BufferHandle buffer_h) {
  auto buffer = to_buffer(buffer_h);
  REI_ASSERT(buffer);

  auto cmd_list = device_resources->prepare_command_list();
  cmd_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(buffer->get_res()));
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
  ID3D12PipelineState* this_pso = shader->pso.Get();
  cmd_list->SetPipelineState(this_pso);

  // Optional: scissor
  if (!cmd.override_area.is_empty()) {
    const RenderArea& area = cmd.override_area;
    D3D12_RECT scissor {};
    scissor.left = float(area.offset_left);
    scissor.top = float(area.offset_top);
    scissor.right = float(area.offset_left + area.width);
    scissor.bottom = float(area.offset_top + area.height);
    cmd_list->RSSetScissorRects(1, &scissor);
  }

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
  bool has_index = bool(cmd.index_buffer);
  bool has_vertex = bool(cmd.vertex_buffer);
  REI_ASSERT(has_index == has_vertex);
  if (has_vertex) {
    UINT index_count;
    {
      auto buffer = to_buffer(cmd.index_buffer);
      auto& ib = buffer->res.get<IndexBuffer>();
      index_count = ib.index_count;
      D3D12_INDEX_BUFFER_VIEW ibv;
      ibv.BufferLocation = ib.res.ptr->GetGPUVirtualAddress();
      ibv.Format = ib.format;
      ibv.SizeInBytes = ib.res.bytesize;
      cmd_list->IASetIndexBuffer(&ibv);
    }
    {
      auto buffer = to_buffer(cmd.vertex_buffer);
      auto& vb = buffer->res.get<VertexBuffer>();
      D3D12_VERTEX_BUFFER_VIEW vbv;
      vbv.BufferLocation = vb.res.ptr->GetGPUVirtualAddress();
      vbv.SizeInBytes = vb.res.bytesize;
      vbv.StrideInBytes = vb.stride;
      cmd_list->IASetVertexBuffers(0, 1, &vbv);
    }
    // Draw
    REI_ASSERT(cmd.index_count == SIZE_MAX || cmd.index_count <= index_count);
    UINT draw_index_count = cmd.index_count == SIZE_MAX ? index_count : UINT(cmd.index_count);
    cmd_list->DrawIndexedInstanced(
      draw_index_count, 1, UINT(cmd.index_offset), UINT(cmd.vertex_offset), 0);

  } else {
    // FIXME better shortcut for blit pass
    cmd_list->IASetVertexBuffers(0, 0, NULL);
    cmd_list->IASetIndexBuffer(NULL);
    cmd_list->DrawInstanced(3, 1, 0, 0);
  }
}

void Renderer::dispatch(const DispatchCommand& cmd) {
  auto shader = to_shader<ComputeShaderData>(cmd.compute_shader);
  REI_ASSERT(shader);
  REI_ASSERT(cmd.dispatch_x * cmd.dispatch_y * cmd.dispatch_z > 0);

  auto cmd_list = device_resources->prepare_command_list();

  ID3D12RootSignature* root_signature = shader->root_signature.Get();
  cmd_list->SetComputeRootSignature(root_signature);
  ID3D12PipelineState* pso = shader->pso.Get();
  cmd_list->SetPipelineState(pso);

  for (UINT i = 0; i < cmd.arguments.size(); i++) {
    auto& h = cmd.arguments[i];
    if (h == c_empty_handle) continue;
    auto arg = to_argument(h);
    REI_ASSERT(arg);
    cmd_list->SetComputeRootDescriptorTable(i, arg->base_descriptor_gpu);
  }

  cmd_list->Dispatch(cmd.dispatch_x, cmd.dispatch_y, cmd.dispatch_z);
}

void Renderer::raytrace(const RaytraceCommand& cmd) {
  auto shader = to_shader<RaytracingShaderData>(cmd.raytrace_shader);
  REI_ASSERT(shader);
  auto buffer = to_buffer(cmd.shader_table);
  REI_ASSERT(buffer && buffer->res.holds<ShaderTableBuffer>());
  auto& shader_table = buffer->res.get<ShaderTableBuffer>();

  auto cmd_list = device_resources->prepare_command_list_dxr();

  // Change render state
  {
    cmd_list->SetComputeRootSignature(shader->root_signature.Get());
    cmd_list->SetPipelineState1(shader->pso.Get());
  }

  // Bing global resource
  {
    for (UINT i = 0; i < cmd.arguments.size(); i++) {
      const auto shader_arg = to_argument(cmd.arguments[i]);
      if (shader_arg) {
        cmd_list->SetComputeRootDescriptorTable(i, shader_arg->base_descriptor_gpu);
      }
    }
  }

  // Dispatch
  {
    auto raygen = shader_table.raygen.get();
    auto hitgroup = shader_table.hitgroup.get();
    auto miss = shader_table.miss.get();

    D3D12_DISPATCH_RAYS_DESC desc = {};
    desc.RayGenerationShaderRecord.StartAddress = raygen->buffer_address();
    desc.RayGenerationShaderRecord.SizeInBytes = raygen->effective_bytewidth();
    desc.MissShaderTable.StartAddress = miss->buffer_address();
    desc.MissShaderTable.SizeInBytes = miss->effective_bytewidth();
    desc.MissShaderTable.StrideInBytes = miss->element_bytewidth();
    desc.HitGroupTable.StartAddress = hitgroup->buffer_address();
    desc.HitGroupTable.SizeInBytes = hitgroup->effective_bytewidth();
    desc.HitGroupTable.StrideInBytes = hitgroup->element_bytewidth();
    desc.Width = UINT(cmd.width);
    desc.Height = UINT(cmd.height);
    desc.Depth = UINT(cmd.depth);
    cmd_list->DispatchRays(&desc);
  }
}

void Renderer::clear_texture(BufferHandle handle, Vec4 clear_value, RenderArea area) {
  auto device = device_resources->device();
  auto cmd_list = device_resources->prepare_command_list();

  auto target = to_buffer(handle);
  REI_ASSERT((target->state & D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
             == D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

  REI_ASSERT(target->res.holds<TextureBuffer>());
  TextureBuffer& tex = target->res.get<TextureBuffer>();
  ID3D12Resource* res = tex.buffer.Get();
  auto d3d_desc = res->GetDesc();
  if ((d3d_desc.Flags
      & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) {
    REI_ERROR(
      "Invalid resource type; The texture should created with allowance of unordred access");
    return;
  }

  D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor;
  D3D12_GPU_DESCRIPTOR_HANDLE gpu_descriptor;
  {
    UINT* cached_descriptor_index = m_texture_clear_descriptor.try_get(target);
    if (cached_descriptor_index) {
      device_resources->cbv_srv_shader_non_visible_heap().get_descriptor_handle(
        *cached_descriptor_index, &cpu_descriptor, &gpu_descriptor);
    } else {
      UINT index = device_resources->cbv_srv_shader_non_visible_heap().alloc(
        &cpu_descriptor, &gpu_descriptor);
      m_texture_clear_descriptor.insert({target, index});

      D3D12_UNORDERED_ACCESS_VIEW_DESC desc {};
      desc.ViewDimension = to_usv_dimension(tex.dimension);
      desc.Format = to_dxgi_format(tex.format);
      device->CreateUnorderedAccessView(res, NULL, &desc, cpu_descriptor);
    }
  }

  FLOAT color[4];
  fill_vec4(color, clear_value);
  D3D12_RECT clear_rect; // NOTE DirectX sreen orientation
  clear_rect.left = area.offset_left;
  clear_rect.top = area.offset_top;
  clear_rect.right = area.offset_left + area.width;
  clear_rect.bottom = area.offset_top + area.height;
  cmd_list->ClearUnorderedAccessViewFloat(
    gpu_descriptor, cpu_descriptor, res, color, 1, &clear_rect);
}

void Renderer::copy_texture(BufferHandle src_h, BufferHandle dst_h, bool revert_state) {
  auto device = device_resources->device();
  auto cmd_list = device_resources->prepare_command_list();

  auto src = to_buffer(src_h);
  auto dst = to_buffer(dst_h);

  ID3D12Resource* src_resource = src->get_res();
  ID3D12Resource* dst_resource = dst->get_res();
  REI_ASSERT(src_resource);
  REI_ASSERT(dst_resource);

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
  auto swapchain = to_swapchain(handle);
  REI_ASSERT(swapchain);

  // check state
  auto rt_buffer = to_buffer(swapchain->get_curr_render_target());
  REI_ASSERT(rt_buffer->state == D3D12_RESOURCE_STATE_PRESENT);

  // Done any writing
  device_resources->flush_command_list();

  // Present and flip
  // TODO check state of swapchain buffer
  if (vsync) {
    swapchain->res_object->Present(1, 0);
  } else {
    swapchain->res_object->Present(0, 0);
  }
  swapchain->advance_curr_rt();

  // Wait
  // FIXME should not wait
  device_resources->flush_command_queue_for_frame();
}

void Renderer::upload_resources() {
  // NOTE: if the cmd_list is closed, we don need to submit it
  bool has_delayed_release = !m_delayed_release.empty() || !m_delayed_release_raw.empty();
  if (is_uploading_resources || has_delayed_release) {
    device_resources->flush_command_list();
    device_resources->flush_command_queue_for_frame();
    is_uploading_resources = false;
  }

  // Done with the delayed relased resources
  // NOTE: both resource type can be release with smart pointer
  m_delayed_release.clear();
  m_delayed_release_raw.clear();
}

void Renderer::build_raytracing_pso(const std::wstring& shader_path,
  const d3d::RaytracingShaderData& shader, ComPtr<ID3D12StateObject>& pso) {
  const RayTraceShaderDesc& meta = shader.meta;

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

void Renderer::build_dxr_acceleration_structure(BuildAccelStruct&& scene, ComPtr<ID3D12Resource>& tlas_buffer) {
  auto device = device_resources->dxr_device();
  auto cmd_list = device_resources->prepare_command_list_dxr();

  // Prepare: get the prebuild info for TLAS
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tl_prebuild = {};
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS toplevel_input = {};
  toplevel_input.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
  toplevel_input.Flags
    = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE; // High quality
  toplevel_input.NumDescs = scene.instance_count;
  toplevel_input.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  toplevel_input.pGeometryDescs = NULL; // instance desc in GPU memory; set latter
  device->GetRaytracingAccelerationStructurePrebuildInfo(&toplevel_input, &tl_prebuild);
  REI_ASSERT(tl_prebuild.ResultDataMaxSizeInBytes > 0);

  // Create scratch space (shared by two AS contruction)
  UINT64 scratch_size = tl_prebuild.ScratchDataSizeInBytes;
  ComPtr<ID3D12Resource> scratch_buffer = create_uav_buffer(device, scratch_size);

  // Allocate BLAS and TLAS buffer
  tlas_buffer = create_accel_struct_buffer(device, tl_prebuild.ResultDataMaxSizeInBytes);

  // TLAS
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlas_desc {};
  ComPtr<ID3D12Resource> instance_buffer;
  {
    // TLAS need the instance descs in GPU
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instance_descs {scene.instance_count};
    for (size_t i = 0; i < scene.instance_count; i++) {
      const bool is_opaque = true;

      // TODO pass tranfrom from model
      D3D12_RAYTRACING_INSTANCE_DESC desc {};
      fill_tlas_instance_transform(desc.Transform, scene.transform[i], model_trans_target);
      desc.InstanceID = scene.instance_id[i];
      desc.InstanceMask = 1;
      desc.InstanceContributionToHitGroupIndex
        = scene.instance_id[i] * 1; // only one entry per instance
      desc.Flags = is_opaque ? D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE
                             : D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE;
      desc.AccelerationStructure = scene.blas[i]->GetGPUVirtualAddress();

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
  
  m_delayed_release_raw.emplace_back(instance_buffer);
  debug_name(instance_buffer, L"TLAS instance buffer");
  m_delayed_release_raw.emplace_back(scratch_buffer);
  debug_name(scratch_buffer, L"TLAS scratch buffer");
}

void Renderer::set_const_buffer(
  BufferHandle handle, size_t index, size_t member, const void* value, size_t width) {
  auto buffer = to_buffer(handle);
  auto& cb = buffer->res.get<ConstBuffer>();
  // validate
  REI_ASSERT(member < cb.layout.size());
  REI_ASSERT(width == get_width(cb.layout[member]));
  // update
  unsigned int local_offset = get_offset(cb.layout, member);
  cb.buffer->update(value, width, index, local_offset);
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

} // namespace rei
