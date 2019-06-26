#ifndef REI_D3D_DEVICE_RESOURCES_H
#define REI_D3D_DEVICE_RESOURCES_H

#if DIRECT3D_ENABLED

#include <vector>
#include <memory>
#include <unordered_map>

#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#include "../common.h"
#include "../algebra.h"
#include "../model.h"
#include "../scene.h"
#include "d3d_common_resources.h"

namespace rei {

namespace d3d {

using Microsoft::WRL::ComPtr;

class Renderer;

// PSO caching
struct PSOKey {
  ID3D12RootSignature* p_root_sign;
  RenderTargetSpec rt_spec;

  //PSOKey(ID3D12RootSignature* p_root_sign, const RenderTargetSpec& rt_spect) : p_root_sign(p_root_sign), rt_spec(rt_spec) {}

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
  DeviceResources(HINSTANCE hInstance);
  ~DeviceResources() = default;

private:
  friend class Renderer;

  HINSTANCE hinstance;

  ComPtr<IDXGIFactory4> dxgi_factory;
  ComPtr<IDXGIAdapter2> dxgi_adapter;
  ComPtr<ID3D12Device> device;

  ComPtr<ID3D12CommandQueue> command_queue;
  ComPtr<ID3D12CommandAllocator> command_alloc;
  ComPtr<ID3D12GraphicsCommandList> draw_command_list;
  ComPtr<ID3D12GraphicsCommandList> upload_command_list;
  bool is_drawing_reset;
  bool is_uploading_reset;

  UINT64 current_frame_fence;
  ComPtr<ID3D12Fence> frame_fence;

  UINT max_shading_buffer_view_num = 128;
  UINT next_shading_buffer_view_index;
  ComPtr<ID3D12DescriptorHeap> shading_buffer_heap;
  UINT shading_buffer_view_inc_size;

  PSOCache pso_cache;

  void compile_shader(const std::wstring& shader_path, ShaderCompileResult& result);
  void create_const_buffers(const ShaderData& shader, ShaderConstBuffers& const_buffers);
  void get_root_signature(ComPtr<ID3D12RootSignature>& root_sign);
  void get_pso(const ShaderData& shader, const RenderTargetSpec& target_spec, ComPtr<ID3D12PipelineState>& pso);

  void create_mesh_buffer(const Mesh& mesh, MeshData& mesh_data);
  void create_debug_mesh_buffer(MeshData& mesh_data);
  void create_mesh_buffer_common(const std::vector<VertexElement>& vertices, const std::vector<std::uint16_t>& indices, MeshData& mesh_res);

  void create_model_buffer(const Model& model, ModelData& model_data);

  void flush_command_queue_for_frame();
};

} // namespace d3d

} // namespace rei

#endif

#endif
