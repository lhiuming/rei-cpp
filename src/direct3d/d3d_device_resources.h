#ifndef REI_D3D_DEVICE_RESOURCES_H
#define REI_D3D_DEVICE_RESOURCES_H

#if DIRECT3D_ENABLED

#include <vector>
#include <memory>

#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#include <d3d11.h> // remove this

#include "../common.h"
#include "../algebra.h"
#include "../model.h"
#include "../scene.h"
#include "d3d_common_resources.h"

namespace rei {

namespace d3d {

using Microsoft::WRL::ComPtr;

class Renderer;

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
  ComPtr<ID3D12GraphicsCommandList> command_list;

  UINT64 current_frame_fence;
  ComPtr<ID3D12Fence> frame_fence;

  template<typename Ele>
  UploadBuffer<Ele> create_const_buffer(UINT64 ele_num) {
    reutrn UnloadBuffer<Ele>(&device, ele_num, true);
  }
  void compile_shader(const std::wstring& shader_path, ShaderCompileResult& result);
  ComPtr<ID3D12PipelineState> get_pso(
    const ShaderData& shader, const RenderTargetSpec& target_spec);

  void create_mesh_buffer(const Mesh& mesh, MeshData& mesh_res);

  void flush_command_queue_for_frame();

  void initialize_default_scene();

  // below are deprecated members

  // D3D interface object
  ID3D11Device* d3d11Device;        // the device abstraction
  ID3D11DeviceContext* d3d11DevCon; // the device context
                                    // Default Shader objects
  ID3D11VertexShader* VS;
  ID3D11PixelShader* PS;
  ID3DBlob* VS_Buffer;
  ID3DBlob* PS_Buffer;

  // Pipeline states objects
  ID3D11RasterizerState* FaceRender; // normal
  ID3D11RasterizerState* LineRender; // with depth bias (to draw cel-line)

  // Rendering objects
  ID3D11InputLayout* vertElementLayout;
  ID3D11Buffer* cbPerFrameBuffer; // shader buffer to hold frame-wide data
  cbPerFrame data_per_frame;      // memory-layouting for frame constant-buffer
  Light g_light;                  // global light-source data, fed to frame-buffer

  ID3D11Buffer* cubeIndexBuffer;
  ID3D11Buffer* cubeVertBuffer;
  ID3D11Buffer* cubeConstBuffer;
  cbPerObject cube_cb_data;
  double cube_rot = 0.0;

};

} // namespace d3d

} // namespace rei

#endif

#endif
