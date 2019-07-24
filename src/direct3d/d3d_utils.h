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
#include "../type_utils.h"

#include "../algebra.h"
#include "../color.h"

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

// Calculate local Root Argument bytesize; useful for shader table layout
UINT get_root_arguments_size(const D3D12_ROOT_SIGNATURE_DESC& signature);

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

inline void fill_color(FLOAT (&dest)[4], const Color& src) {
  dest[0] = src.r;
  dest[1] = src.g;
  dest[2] = src.b;
  dest[3] = src.a;
}

// Wrapper class for a reused upload buffer,
// with handy operation
class UploadBuffer : NoCopy {
public:
  struct Hasher {
    std::size_t operator()(const UploadBuffer& value) const {
      return std::uintptr_t(value.resource());
    }
  };

  struct Layout {
    const UINT element_width = -1;
    const UINT element_alignment = 0;
    const UINT buffer_alignment = 0;
    Layout(UINT ele_width) : element_width(ele_width) {}
    Layout(UINT ele_width, UINT ele_align, UINT buf_align)
        : element_width(ele_width), element_alignment(ele_align), buffer_alignment(buf_align) {}
  };

  UploadBuffer(ID3D12Device& device, Layout layout, UINT element_num,
    D3D12_RESOURCE_STATES init_states = D3D12_RESOURCE_STATE_GENERIC_READ)
      : m_layout(layout),
        m_address_start_alignment(
          max(layout.element_alignment, layout.buffer_alignment)), // assume they are compatible
        m_element_num(element_num),
        m_element_bytewidth(align_bytesize(layout.element_width, layout.element_alignment)),
        m_effective_bytewidth(element_num * m_element_bytewidth),
        m_padded_bytewidth(m_effective_bytewidth + m_address_start_alignment),
        m_init_state(init_states) {
    HRESULT hr;

    // ... Life should be easy
    REI_ASSERT(
      layout.element_alignment == 0 || layout.buffer_alignment % layout.element_alignment == 0);

    D3D12_HEAP_PROPERTIES heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE;
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(m_padded_bytewidth);
    D3D12_CLEAR_VALUE* clear_value = nullptr;
    hr = device.CreateCommittedResource(
      &heap_prop, heap_flags, &desc, init_states, clear_value, IID_PPV_ARGS(&buffer));
    REI_ASSERT(SUCCEEDED(hr));

    // calculate the offset needed to align the gpu address
    D3D12_GPU_VIRTUAL_ADDRESS buf_addr = buffer->GetGPUVirtualAddress();
    D3D12_GPU_VIRTUAL_ADDRESS aligned_buf_addr
      = align_bytesize(buf_addr, m_address_start_alignment);
    REI_ASSERT(aligned_buf_addr >= buf_addr);
    buffer_offset = aligned_buf_addr - buf_addr;

    UINT subres = 0;
    D3D12_RANGE* range = nullptr;
    hr = buffer->Map(subres, range, (void**)(&mapped_memory));
    REI_ASSERT(SUCCEEDED(hr));
  }

  UploadBuffer(
    ID3D12Device& device, UINT element_width, bool is_const_buffer = true, UINT element_num = 1)
      : UploadBuffer(device, Layout(element_width, is_const_buffer ? 256 : 0, 0), element_num) {}

  ~UploadBuffer() {
    UINT subres = 0;
    D3D12_RANGE* range = nullptr;
    buffer->Unmap(subres, range);
  }

  bool operator==(const UploadBuffer& other) const { return buffer == other.buffer; }

  UINT size() const { return m_element_num; }
  UINT64 element_bytewidth() const { return m_element_bytewidth; }
  UINT64 effective_bytewidth() const { return m_effective_bytewidth; }
  UINT64 total_bytewidth() const { return m_padded_bytewidth; }

  D3D12_RESOURCE_STATES init_state() const { return m_init_state; }

  void update(const void* src, size_t bytecount, UINT index, UINT local_offset) const {
    REI_ASSERT(index < m_element_num);
    REI_ASSERT(local_offset + bytecount <= m_element_bytewidth);
    UINT64 offset = buffer_offset + index * m_element_bytewidth + local_offset;
    memcpy(mapped_memory + offset, (byte*)src, bytecount);
  }

  template <typename Ele>
  void update(const Ele& value, UINT index, UINT local_offset = 0) const {
    update(&value, 0, sizeof(Ele), 0, index, local_offset);
  }

  ID3D12Resource* resource() const { return buffer.Get(); }
  D3D12_GPU_VIRTUAL_ADDRESS buffer_address(UINT element_index = 0) const {
    REI_ASSERT(element_index < m_element_num);
    D3D12_GPU_VIRTUAL_ADDRESS address_start = buffer.Get()->GetGPUVirtualAddress() + buffer_offset;
    return address_start + element_index * m_element_bytewidth;
  }

protected:
  const Layout m_layout;
  const UINT64 m_address_start_alignment = 0;
  const UINT m_element_num = 0;
  const UINT64 m_element_bytewidth;
  const UINT64 m_effective_bytewidth = 0;
  const UINT64 m_padded_bytewidth = 0;

  const D3D12_RESOURCE_STATES m_init_state;

  UINT64 buffer_offset = 0;
  ComPtr<ID3D12Resource> buffer;
  byte* mapped_memory = nullptr;
};

// Wrapper class to help building shader table
// Each element in the upload buffer is a shader record,
// with element_width = shader_id_width + root_arguments_width
class ShaderTable : public UploadBuffer {
  // Typed shader record
  template <typename RootArgs>
  struct ShaderRecord {
    BYTE shader_id_data[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
    RootArgs root_arguments;
  };

  struct ShaderTableLayout : UploadBuffer::Layout {
    ShaderTableLayout(UINT args_width)
        : Layout(args_width + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES,
          D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT,
          D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) {}
  };

public:
  ShaderTable(ID3D12Device& device, UINT root_args_width, UINT num)
      : UploadBuffer(device, ShaderTableLayout(root_args_width), num) {}

  template <typename RootArgs>
  ShaderTable(ID3D12Device& device, UINT num)
      : ShaderTable(device, sizeof(ShaderRecord<RootArgs>), num) {}

  void update(const void* shader_id, const void* args, size_t bytecount, UINT index,
    UINT local_offset) const {
    size_t offset = buffer_offset + m_element_bytewidth * index + local_offset;
    memcpy(mapped_memory + offset, shader_id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    if (bytecount > 0) {
      offset += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
      memcpy(mapped_memory + offset, args, bytecount);
    }
  }

  template <typename RootArgs>
  void update(void* shader_id, RootArgs&& args, UINT index) const {
    Record record {};
    memcpy(&record.shader_id_data, shader_id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    record.root_arguments = std::move(args);
    Base::update(record, index);
  }

private:
};

// Naive descriptor allocator
class NaiveDescriptorHeap : NoCopy {
public:
  NaiveDescriptorHeap() = default;
  NaiveDescriptorHeap(NaiveDescriptorHeap&& other) = default;
  NaiveDescriptorHeap& operator=(NaiveDescriptorHeap&& other) = default;

  NaiveDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT capacity)
      : max_descriptor_num(capacity),
        m_descriptor_size(device->GetDescriptorHandleIncrementSize(type)) {
    // Automatic flag
    D3D12_DESCRIPTOR_HEAP_FLAGS flag = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
      flag = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    }

    // Create buffer-type descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.Type = type;
    heap_desc.NumDescriptors = max_descriptor_num;
    heap_desc.Flags = flag;
    heap_desc.NodeMask = 0; // sinlge GPU
    HRESULT hr = device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&m_descriotpr_heap));
    REI_ASSERT(SUCCEEDED(hr));
    next_descriptor_index = 0;
  }

  ID3D12DescriptorHeap* get_ptr() const { return m_descriotpr_heap.Get(); }
  ID3D12DescriptorHeap* const* get_ptr_addr() const { return m_descriotpr_heap.GetAddressOf(); }

  UINT cnv_srv_descriptor_size() const { return m_descriptor_size; };

  UINT alloc(UINT count, D3D12_CPU_DESCRIPTOR_HANDLE* cpu_descrioptor = nullptr,
    D3D12_GPU_DESCRIPTOR_HANDLE* gpu_descriptor = nullptr) {
    ID3D12DescriptorHeap* heap = m_descriotpr_heap.Get();
    REI_ASSERT(heap);
    REI_ASSERT(next_descriptor_index + count < max_descriptor_num);
    if (cpu_descrioptor) {
      *cpu_descrioptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        heap->GetCPUDescriptorHandleForHeapStart(), INT(next_descriptor_index), m_descriptor_size);
    }
    if (gpu_descriptor) {
      *gpu_descriptor = CD3DX12_GPU_DESCRIPTOR_HANDLE(
        heap->GetGPUDescriptorHandleForHeapStart(), INT(next_descriptor_index), m_descriptor_size);
    }
    UINT head_alloc_index = next_descriptor_index;
    next_descriptor_index += count;
    return head_alloc_index;
  }

  UINT alloc(D3D12_CPU_DESCRIPTOR_HANDLE* cpu_descrioptor = nullptr,
    D3D12_GPU_DESCRIPTOR_HANDLE* gpu_descriptor = nullptr) {
    return alloc(1, cpu_descrioptor, gpu_descriptor);
  }

protected:
  UINT max_descriptor_num = 0;
  UINT m_descriptor_size = UINT_MAX;
  UINT next_descriptor_index = 0;
  ComPtr<ID3D12DescriptorHeap> m_descriotpr_heap;
};

} // namespace d3d

} // namespace rei

#endif

#endif
