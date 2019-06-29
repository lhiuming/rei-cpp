#ifndef REI_DIRECT3D_D3D_UTILS_H
#define REI_DIRECT3D_D3D_UTILS_H

#if DIRECT3D_ENABLED

#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <dxcapi.h>
#include <windows.h>
#include <wrl.h>

#include "../debug.h"

namespace rei {
namespace d3d {

using Microsoft::WRL::ComPtr;

constexpr char c_raytracing_shader_target[] = "lib_6_3";
constexpr wchar_t c_raytracing_shader_target_w[] = L"lib_6_3";

ComPtr<ID3DBlob> compile_shader(
  const wchar_t* shader_path, const char* entrypoint, const char* target);

ComPtr<IDxcBlob> compile_dxr_shader(const wchar_t* shader_path, const wchar_t* entrypoint);

inline UINT64 align_cb_bytesize(size_t bytesize) {
  const UINT64 CB_ALIGNMENT_WIDTH = 256;
  const UINT64 CB_ALIGNMENT_MASK = ~255;
  return (bytesize + CB_ALIGNMENT_WIDTH) & CB_ALIGNMENT_MASK;
}

// Create a simple upload buffer, and populate it with cpu data (optionally)
ComPtr<ID3D12Resource> create_upload_buffer(
  ID3D12Device* device, size_t bytesize, const void* data = nullptr);

// Create a simple upload buffer, with size automatically calculated
template <typename T>
ComPtr<ID3D12Resource> create_upload_buffer(
  ID3D12Device* device, size_t element_num, bool is_const_buffer = false) {
  UINT64 element_bsize = sizeof(T);
  if (is_const_buffer) { element_bsize = align_cb_bytesize(element_bsize); }
  UINT64 bytesize = element_bsize * element_num;
  return create_upload_buffer(device, bytesize);
}

// Create a simple upload buffer and populate with given data
template <typename T>
ComPtr<ID3D12Resource> create_upload_buffer(ID3D12Device* device, T* elements, size_t element_num) {
  UINT64 element_bsize = sizeof(T);
  UINT64 bytesize = element_bsize * element_num;
  return create_upload_buffer(device, bytesize, elements);
}

// Create a default buffer with custom init state and flags
ComPtr<ID3D12Resource> create_default_buffer(
  ID3D12Device* device, size_t bytesize, D3D12_RESOURCE_STATES init_state, D3D12_RESOURCE_FLAGS flags);

// Create a simple default buffer
inline ComPtr<ID3D12Resource> create_default_buffer(ID3D12Device* device, size_t bytesize) {
  D3D12_RESOURCE_STATES init_state = D3D12_RESOURCE_STATE_COMMON;
  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
  return create_default_buffer(device, bytesize, init_state, flags);
}

// Create a simple default buffer ready for accessing as UAV
inline ComPtr<ID3D12Resource> create_uav_buffer(ID3D12Device* device, size_t bytesize,
  D3D12_RESOURCE_STATES init_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  return create_default_buffer(device, bytesize, init_state, flags);
}

inline ComPtr<ID3D12Resource> craete_accel_struct_buffer(ID3D12Device* device, size_t bytesize,
  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) {
  constexpr D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
  // acceleration structure must allow unordered access
  // ref: https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html
  flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  return create_default_buffer(device, bytesize, state, flags);
}

// Upload the data to a newly created upload buffer, and copy to the destination default buffer
ComPtr<ID3D12Resource> upload_to_default_buffer(ID3D12Device* device,
  ID3D12GraphicsCommandList* cmd_list, const void* data, size_t bytesize,
  ID3D12Resource* dest_buffer);

// Wrapper class for a reused upload buffer,
// with handy operation
template <typename Ele>
class UploadBuffer : NoCopy {
public:
  struct Hasher {
    std::size_t operator()(const UploadBuffer& value) const {
      return std::uintptr_t(value.resource());
    }
  };

  UploadBuffer(ID3D12Device& device, UINT64 element_num, bool is_const_buffer)
      : m_element_num(element_num), m_is_const_buffer(is_const_buffer) {
    // align the buffer size
    m_element_bytesize = sizeof(Ele);
    if (is_const_buffer) { m_element_bytesize = align_cb_bytesize(m_element_bytesize); }

    HRESULT hr;

    D3D12_HEAP_PROPERTIES heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE;
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(m_element_bytesize * element_num);
    D3D12_CLEAR_VALUE* clear_value = nullptr;
    hr = device.CreateCommittedResource(&heap_prop, heap_flags, &desc,
      D3D12_RESOURCE_STATE_GENERIC_READ, clear_value, IID_PPV_ARGS(&buffer));
    REI_ASSERT(SUCCEEDED(hr));

    UINT subres = 0;
    D3D12_RANGE* range = nullptr;
    hr = buffer->Map(subres, range, (void**)(&mapped_memory));
    REI_ASSERT(SUCCEEDED(hr));
  }

  ~UploadBuffer() {
    UINT subres = 0;
    D3D12_RANGE* range = nullptr;
    buffer->Unmap(subres, range);
  }

  bool operator==(const UploadBuffer& other) const { return buffer == other.buffer; }

  bool is_const_buffer() const { return m_is_const_buffer; }
  UINT size() const { return m_element_num; }
  UINT element_bytesize() const { return m_element_bytesize; }

  void update(const Ele& value, UINT index = 0) const {
    REI_ASSERT(index < m_element_num);
    memcpy(mapped_memory + index * m_element_bytesize, &value, sizeof(Ele));
  }

  ID3D12Resource* resource() const { return buffer.Get(); }
  D3D12_GPU_VIRTUAL_ADDRESS buffer_address(UINT element_index = 0) const {
    REI_ASSERT(element_index < m_element_num);
    return buffer.Get()->GetGPUVirtualAddress() + element_index * m_element_bytesize;
  }

private:
  UINT m_element_num;
  UINT m_element_bytesize;
  bool m_is_const_buffer;

  ComPtr<ID3D12Resource> buffer;
  byte* mapped_memory;
};

} // namespace d3d

} // namespace rei

#endif

#endif
