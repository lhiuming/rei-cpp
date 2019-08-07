#if DIRECT3D_ENABLED

#include "d3d_utils.h";

namespace rei {

namespace d3d {

ComPtr<ID3DBlob> compile_shader(
  const wchar_t* shader_path, const char* entrypoint, const char* target, const D3D_SHADER_MACRO* macros) {
  UINT compile_flags = 0;
#if defined(DEBUG) || !defined(NDEBUG)
  compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

  ComPtr<ID3DBlob> btyecode;
  ComPtr<ID3DBlob> error_msg;

  HRESULT hr = D3DCompileFromFile(shader_path, // shader file path
    macros,                                    // preprocessors
    D3D_COMPILE_STANDARD_FILE_INCLUDE,         // "include file with relative path"
    entrypoint,                                // e.g. "VS" or "PS" or "main" or others
    target,                                    // "vs_5_0" or "ps_5_0" or similar
    compile_flags,                             // options
    0,                                         // more options
    &btyecode,                                 // result
    &error_msg                                 // message if error
  );

  if (FAILED(hr)) {
    if (error_msg) {
      char* err_str = (char*)error_msg->GetBufferPointer();
      REI_ERROR(err_str);
    } else {
      REI_ERROR("Shader compilation failed with no error messagable available.");
    }
  }

  return btyecode;
}

ComPtr<ID3DBlob> compile_shader(const char* shader_name, const char* shader_text, const char* entrypoint, const char* target) {

  UINT compile_flags = 0;
#if defined(DEBUG) || !defined(NDEBUG)
  compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

  ComPtr<ID3DBlob> btyecode;
  ComPtr<ID3DBlob> error_msg;

  HRESULT hr = D3DCompile(shader_text, strlen(shader_text), shader_name,
    nullptr,                                    // preprocessors
    D3D_COMPILE_STANDARD_FILE_INCLUDE,         // "include file with relative path"
    entrypoint,                                // e.g. "VS" or "PS" or "main" or others
    target,                                    // "vs_5_0" or "ps_5_0" or similar
    compile_flags,                             // options
    0,                                         // more options
    &btyecode,                                 // result
    &error_msg                                 // message if error
  );

  if (FAILED(hr)) {
    if (error_msg) {
      char* err_str = (char*)error_msg->GetBufferPointer();
      REI_ERROR(err_str);
    } else {
      REI_ERROR("Shader compilation failed with no error messagable available.");
    }
  }

  return btyecode;
}

ComPtr<IDxcBlob> compile_dxr_shader(const wchar_t* shader_path, const wchar_t* entrypoint) {
  HRESULT hr;

  // read
  ComPtr<IDxcLibrary> lib;
  hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&lib));
  REI_ASSERT(SUCCEEDED(hr));
  ComPtr<IDxcIncludeHandler> includer;
  hr = lib->CreateIncludeHandler(&includer);
  REI_ASSERT(SUCCEEDED(hr));

  ComPtr<IDxcBlobEncoding> source;
  hr = lib->CreateBlobFromFile(shader_path, nullptr, &source);
  REI_ASSERT(SUCCEEDED(hr));

  // compile
  ComPtr<IDxcCompiler2> compiler;
  hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
  REI_ASSERT(SUCCEEDED(hr));
  ComPtr<IDxcOperationResult> result;
  ComPtr<IDxcBlob> debug;
  wchar_t* debug_name;
  hr = compiler->CompileWithDebug(source.Get(), shader_path, entrypoint,
    c_raytracing_shader_target_w, nullptr, 0, nullptr, 0, includer.Get(), &result, &debug_name,
    &debug);
  REI_ASSERT(SUCCEEDED(hr));

  ComPtr<IDxcBlob> blob;
  result->GetStatus(&hr);
  if (FAILED(hr)) {
    ComPtr<IDxcBlobEncoding> error;
    result->GetErrorBuffer(&error);
    auto msg = (char*)error->GetBufferPointer();
    REI_ERROR(msg);
    return nullptr;
  }

  hr = result->GetResult(&blob);
  REI_ASSERT(SUCCEEDED(hr));
  return blob;
}

// ref: https://docs.microsoft.com/en-us/windows/win32/direct3d12/root-signature-limits
UINT get_root_arguments_size(const D3D12_ROOT_SIGNATURE_DESC& signature) {
  UINT sum = 0;
  for (UINT param_i = 0; param_i < signature.NumParameters; param_i++) {
    const D3D12_ROOT_PARAMETER& param = signature.pParameters[param_i];
    const D3D12_ROOT_PARAMETER_TYPE p_type = param.ParameterType;
    switch (p_type) {
      case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
        sum += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
        break;
      case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
        sum += param.Constants.Num32BitValues * sizeof(int32_t);
        break;
      case D3D12_ROOT_PARAMETER_TYPE_CBV:
      case D3D12_ROOT_PARAMETER_TYPE_SRV:
      case D3D12_ROOT_PARAMETER_TYPE_UAV:
        sum += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        break;
      default:
        REI_ERROR("Unexpected Parameter type. Probably due to memory corruption.");
        return 0;
    }
  }
  return sum;
}

ComPtr<ID3D12Resource> create_upload_buffer(
  ID3D12Device* device, size_t bytesize, const void* data) {
  REI_ASSERT(device);
  if (REI_WARNINGIF(bytesize == 0)) bytesize = 1;

  D3D12_HEAP_PROPERTIES heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE;
  D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bytesize);
  D3D12_RESOURCE_STATES init_state = D3D12_RESOURCE_STATE_GENERIC_READ; // required in upload heap
  D3D12_CLEAR_VALUE* p_clear_value = nullptr; // optional
  ComPtr<ID3D12Resource> upload_buffer;
  HRESULT hr = device->CreateCommittedResource(
    &heap_prop, heap_flags, &desc, init_state, p_clear_value, IID_PPV_ARGS(&upload_buffer));
  REI_ASSERT(SUCCEEDED(hr));

  if (data != nullptr) {
    void* system_memory;
    upload_buffer->Map(0, nullptr, &system_memory);
    memcpy(system_memory, data, bytesize);
    upload_buffer->Unmap(0, nullptr);
  }

  return upload_buffer;
}

ComPtr<ID3D12Resource> create_default_buffer(ID3D12Device* device, size_t bytesize,
  D3D12_RESOURCE_STATES init_state, D3D12_RESOURCE_FLAGS flags) {
  REI_ASSERT(device);
  REI_ASSERT(bytesize > 0);

  D3D12_HEAP_PROPERTIES heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bytesize, flags);
  D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE;
  D3D12_CLEAR_VALUE* p_clear_value = nullptr; // optional
  ComPtr<ID3D12Resource> result;
  HRESULT hr = device->CreateCommittedResource(
    &heap_prop, heap_flags, &desc, init_state, p_clear_value, IID_PPV_ARGS(&result));
  REI_ASSERT(SUCCEEDED(hr));
  return result;
};

ComPtr<ID3D12Resource> upload_to_default_buffer(ID3D12Device* device,
  ID3D12GraphicsCommandList* cmd_list, const void* data, size_t bytesize,
  ID3D12Resource* dest_buffer) {
  REI_ASSERT(device);
  REI_ASSERT(cmd_list);
  REI_ASSERT(data);
  REI_ASSERT(dest_buffer);

  // create upalod buffer
  ComPtr<ID3D12Resource> upload_buffer = create_upload_buffer(device, bytesize);

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
  UpdateSubresources(cmd_list, dest_buffer, upload_buffer.Get(), upload_offset,
    upload_first_sub_res, 1, &sub_res_data);

  // post-upload transition
  D3D12_RESOURCE_BARRIER post_upload = CD3DX12_RESOURCE_BARRIER::Transition(
    dest_buffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
  cmd_list->ResourceBarrier(1, &post_upload);

  return upload_buffer;
};

} // namespace d3d

} // namespace rei

#endif
