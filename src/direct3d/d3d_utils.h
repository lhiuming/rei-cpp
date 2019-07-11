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
#include "../algebra.h"

namespace rei {
namespace d3d {

using Microsoft::WRL::ComPtr;

constexpr char c_raytracing_shader_target[] = "lib_6_3";
constexpr wchar_t c_raytracing_shader_target_w[] = L"lib_6_3";

ComPtr<ID3DBlob> compile_shader(
  const wchar_t* shader_path, const char* entrypoint, const char* target);

ComPtr<IDxcBlob> compile_dxr_shader(const wchar_t* shader_path, const wchar_t* entrypoint);

constexpr inline UINT64 align_bytesize(UINT64 bytesize, UINT align_width) {
  if (align_width == 0) return bytesize;
  const UINT64 align_comp = align_width - 1;
  const UINT64 align_mask = ~align_comp;
  return (bytesize + align_comp) & align_mask;
}

constexpr inline UINT64 align_cb_bytesize(size_t bytesize) {
  const UINT64 CB_ALIGNMENT_WIDTH = 256;
  return align_bytesize(bytesize, CB_ALIGNMENT_WIDTH);
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
ComPtr<ID3D12Resource> create_default_buffer(ID3D12Device* device, size_t bytesize,
  D3D12_RESOURCE_STATES init_state, D3D12_RESOURCE_FLAGS flags);

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

// Create a simple default buffer ready for accessing as UAV 2d texture
inline ComPtr<ID3D12Resource> create_uav_texture2d(ID3D12Device* device, size_t bytesize,
  D3D12_RESOURCE_STATES init_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  return create_default_buffer(device, bytesize, init_state, flags);
}

inline ComPtr<ID3D12Resource> create_accel_struct_buffer(ID3D12Device* device, size_t bytesize,
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

// Fill the row-major 3x4 matrix appears in TLAS interface
inline void fill_tlas_instance_transform(
  FLOAT (&dest)[3][4], const Mat4& src, VectorTarget vec_target = VectorTarget::Column) {
  if (vec_target == VectorTarget::Column) {
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 4; j++)
        dest[i][j] = float(src(i, j));
  } else { // flip, since DXR TLAS expect a matrix for column vector
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 4; j++)
        dest[i][j] = float(src(j, i));
  }
}

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

  UploadBuffer(ID3D12Device& device, UINT element_num,
    UINT element_alignment = 0, UINT64 buffer_alignment = 0,
    D3D12_RESOURCE_STATES init_states = D3D12_RESOURCE_STATE_GENERIC_READ)
      : m_element_num(element_num),
        m_element_bytewidth(align_bytesize(sizeof(Ele), element_alignment)),
        m_total_bytewidth(align_bytesize(element_num * m_element_bytewidth, buffer_alignment)),
        m_buffer_address_alignment(buffer_alignment) {
    HRESULT hr;

    D3D12_HEAP_PROPERTIES heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE;
    D3D12_RESOURCE_DESC desc
      = CD3DX12_RESOURCE_DESC::Buffer(m_total_bytewidth);
    D3D12_CLEAR_VALUE* clear_value = nullptr;
    hr = device.CreateCommittedResource(
      &heap_prop, heap_flags, &desc, init_states, clear_value, IID_PPV_ARGS(&buffer));
    REI_ASSERT(SUCCEEDED(hr));

    UINT subres = 0;
    D3D12_RANGE* range = nullptr;
    hr = buffer->Map(subres, range, (void**)(&mapped_memory));
    REI_ASSERT(SUCCEEDED(hr));
  }

  UploadBuffer(ID3D12Device& device, UINT element_num, bool is_const_buffer)
      : UploadBuffer(
          device, element_num, is_const_buffer ? 256 : 0, 0) {}

  ~UploadBuffer() {
    UINT subres = 0;
    D3D12_RANGE* range = nullptr;
    buffer->Unmap(subres, range);
  }

  bool operator==(const UploadBuffer& other) const { return buffer == other.buffer; }

  UINT size() const { return m_element_num; }
  UINT64 element_bytewidth() const { return m_element_bytewidth; }
  UINT64 bytewidth() const { return m_total_bytewidth; }

  void update(const Ele& value, UINT index = 0) const {
    REI_ASSERT(index < m_element_num);
    memcpy(mapped_memory + index * m_element_bytewidth, &value, sizeof(Ele));
  }

  ID3D12Resource* resource() const { return buffer.Get(); }
  D3D12_GPU_VIRTUAL_ADDRESS buffer_address(UINT element_index = 0) const {
    REI_ASSERT(element_index < m_element_num);
    D3D12_GPU_VIRTUAL_ADDRESS address_start = buffer.Get()->GetGPUVirtualAddress();
    address_start = align_bytesize(address_start, m_buffer_address_alignment);
    return address_start + element_index * m_element_bytewidth;
  }

private:
  const UINT m_element_num = 0;
  const UINT64 m_element_bytewidth = 0;
  const UINT64 m_total_bytewidth = 0;
  const UINT64 m_buffer_address_alignment = 0;

  ComPtr<ID3D12Resource> buffer;
  byte* mapped_memory = nullptr;
};

// Wrapper class to help building shader table
template<typename RootArgs>
struct ShaderEntry {
  BYTE shader_id_data[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
  RootArgs root_arguments;
};
template<typename RootArgs>
class ShaderTable : public UploadBuffer<ShaderEntry<RootArgs>> {
  using Base = UploadBuffer<ShaderEntry<RootArgs>>;
  using Record = ShaderEntry<RootArgs>;

public:
  ShaderTable(ID3D12Device& device, UINT size)
      : Base(device, size, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,
          D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) {
    // empty
  }

  void update(void* shader_id, RootArgs&& args, UINT index) const {
    Record entry {};
    memcpy(&entry.shader_id_data, shader_id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    entry.root_arguments = std::move(args);
    Base::update(entry, index);
  }

private:
};

} // namespace d3d

} // namespace rei

#endif

#endif
