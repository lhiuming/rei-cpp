#ifndef REI_D3D_DEVICE_RESOURCES_H
#define REI_D3D_DEVICE_RESOURCES_H

#if DIRECT3D_ENABLED

#include <memory>
#include <unordered_map>
#include <vector>

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <windows.h>
#include <wrl.h>

#include "../algebra.h"
#include "../common.h"
#include "../model.h"
#include "../scene.h"
#include "d3d_common_resources.h"

namespace rei {

namespace d3d {

using Microsoft::WRL::ComPtr;

// PSO caching
struct PSOKey {
  ID3D12RootSignature* p_root_sign;
  RenderTargetSpec rt_spec;

  // PSOKey(ID3D12RootSignature* p_root_sign, const RenderTargetSpec& rt_spect) :
  // p_root_sign(p_root_sign), rt_spec(rt_spec) {}

  bool operator==(const PSOKey& other) const {
    return p_root_sign == other.p_root_sign && rt_spec == other.rt_spec;
  }
};

struct PSOKeyHasher {
  std::size_t operator()(PSOKey const& k) const noexcept {
    std::size_t hash0 = (uintptr_t)k.p_root_sign;
    std::size_t hash1 = k.rt_spec.simple_hash();
    // TODO use some better hash combine scheme
    return hash0 ^ hash1;
  }
};

using PSOCache = std::unordered_map<PSOKey, ComPtr<ID3D12PipelineState>, PSOKeyHasher>;

class DeviceResources : NoCopy {
public:
  struct Options {
    bool is_dxr_enabled = true;
  };

  DeviceResources(HINSTANCE hInstance, Options options = {});
  ~DeviceResources() = default;

  IDXGIFactory4& dxgi_factory() const { return *m_dxgi_factory.Get(); }
  IDXGIAdapter2& dxgi_adapter() const { return *m_dxgi_adapter.Get(); }
  ID3D12Device& device() const { return *m_device.Get(); }

  ID3D12CommandQueue& command_queue() const { return *m_command_queue.Get(); }
  //ID3D12CommandAllocator& command_alloc() const { return *m_command_alloc.Get(); }
  //ID3D12GraphicsCommandList& command_list() const { return *m_draw_command_list.Get(); }

  ID3D12DescriptorHeap& descriptor_heap() const { return common_descriptor_heap(); }
  ID3D12DescriptorHeap& common_descriptor_heap() const { return *shading_buffer_heap.Get(); }
  UINT alloc_descriptor() {
    ID3D12DescriptorHeap* heap = shading_buffer_heap.Get();
    REI_ASSERT(heap);
    UINT alloc_index = next_shading_buffer_view_index++;
    REI_ASSERT(alloc_index < max_shading_buffer_view_num);
    CD3DX12_CPU_DESCRIPTOR_HANDLE descriptor {
      heap->GetCPUDescriptorHandleForHeapStart(), (INT)alloc_index, shading_buffer_view_inc_size};
    return alloc_index;
  }

  void compile_shader(const std::wstring& shader_path, ShaderCompileResult& result);
  void create_const_buffers(const ShaderData& shader, ShaderConstBuffers& const_buffers);
  void get_root_signature(ComPtr<ID3D12RootSignature>& root_sign);
  void create_root_signature(const D3D12_ROOT_SIGNATURE_DESC& root_desc, ComPtr<ID3D12RootSignature>& root_sign);
  void get_pso(const ShaderData& shader, const RenderTargetSpec& target_spec,
    ComPtr<ID3D12PipelineState>& pso);

  void create_mesh_buffer(const Mesh& mesh, MeshData& mesh_data);
  void create_debug_mesh_buffer(MeshData& mesh_data);

  void create_model_buffer(const Model& model, ModelData& model_data);

  ID3D12GraphicsCommandList& prepare_command_list(ID3D12PipelineState* init_pso = nullptr);
  ID3D12GraphicsCommandList4& prepare_command_list_dxr(ID3D12PipelineState* init_pso = nullptr) {
    prepare_command_list();
    return *m_dxr_command_list.Get();
  }
  void flush_command_list();
  void flush_command_queue_for_frame();

private:
  HINSTANCE hinstance;

  // Flatten options
  const bool is_dxr_enabled;

  ComPtr<IDXGIFactory4> m_dxgi_factory;
  ComPtr<IDXGIAdapter2> m_dxgi_adapter;
  ComPtr<ID3D12Device> m_device;
  ComPtr<ID3D12Device5> m_dxr_device; // newer interface of m_device

  ComPtr<ID3D12CommandQueue> m_command_queue;
  ComPtr<ID3D12CommandAllocator> m_command_alloc;
  ComPtr<ID3D12GraphicsCommandList> m_command_list;
  ComPtr<ID3D12GraphicsCommandList4> m_dxr_command_list; // newer interface of m_command_list
  bool is_using_cmd_list;

  UINT64 current_frame_fence;
  ComPtr<ID3D12Fence> frame_fence;

  // Naive descriptor allocator
  UINT max_shading_buffer_view_num = 128;
  UINT next_shading_buffer_view_index;
  ComPtr<ID3D12DescriptorHeap> shading_buffer_heap;
  UINT shading_buffer_view_inc_size;

  PSOCache pso_cache;

  void create_mesh_buffer_common(const std::vector<VertexElement>& vertices,
    const std::vector<std::uint16_t>& indices, MeshData& mesh_res);
};

} // namespace d3d

} // namespace rei

#endif

#endif
