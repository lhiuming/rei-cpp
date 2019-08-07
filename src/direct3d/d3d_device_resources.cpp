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
using std::move;
using std::runtime_error;
using std::shared_ptr;
using std::string;
using std::vector;
using std::wstring;

namespace rei {

namespace d3d {

DeviceResources::DeviceResources(HINSTANCE h_inst, Options opt)
    : hinstance(h_inst), is_dxr_enabled(opt.is_dxr_enabled) {
  HRESULT hr;

  UINT dxgi_factory_flags = 0;
#if DEBUG
  {
    // d3d12 debug layer
    // ComPtr<ID3D12Debug> debug_controller;
    ComPtr<ID3D12Debug1> debug_controller;
    REI_ASSERT(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))));
    debug_controller->EnableDebugLayer();
    // deeper debug, might be alot slower
    // debug_controller->SetEnableGPUBasedValidation(true);
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
  REI_ASSERT(SUCCEEDED(m_device->CreateCommandList(node_mask, list_type, m_command_alloc.Get(),
    init_pip_state, IID_PPV_ARGS(&m_command_list_legacy))));
  REI_ASSERT(SUCCEEDED(m_command_list_legacy->QueryInterface(IID_PPV_ARGS(&m_command_list))));
  m_command_list->Close();
  is_using_cmd_list = false;

  // Create fence
  D3D12_FENCE_FLAGS fence_flags = D3D12_FENCE_FLAG_NONE;
  REI_ASSERT(SUCCEEDED(m_device->CreateFence(0, fence_flags, IID_PPV_ARGS(&frame_fence))));
  current_frame_fence = 0;

  // Create  descriptor heaps
  m_cbv_srv_heap = NaiveDescriptorHeap(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 256,
    D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
  m_cbv_srv_shader_nonvisible_heap
    = NaiveDescriptorHeap(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 64);
  m_rtv_heap = NaiveDescriptorHeap(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 8);
  m_dsv_heap = NaiveDescriptorHeap(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 8);
}

void DeviceResources::get_root_signature(
  const D3D12_ROOT_SIGNATURE_DESC& root_desc, ComPtr<ID3D12RootSignature>& root_sign) {
  /*
   * TODO maybe cache the root signatures
   * NOTE: d3d12 seems already do caching root signature for identical DESC
   */
  create_root_signature(root_desc, root_sign);
}

void DeviceResources::create_root_signature(
  const D3D12_ROOT_SIGNATURE_DESC& root_desc, ComPtr<ID3D12RootSignature>& root_sign) {
  HRESULT hr;
  ComPtr<ID3DBlob> root_sign_blob;
  ComPtr<ID3DBlob> error_blob;
  hr = D3D12SerializeRootSignature(
    &root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_sign_blob, &error_blob);
  if (!SUCCEEDED(hr)) {
    if (error_blob != nullptr) {
      const char* msg = (char*)error_blob->GetBufferPointer();
      REI_ERROR(msg);
    } else {
      REI_ERROR("Root Signature creation fail with not error message");
    }
    root_sign = nullptr;
    return;
  }
  UINT node_mask = 0; // single GPU
  hr = m_device->CreateRootSignature(node_mask, root_sign_blob->GetBufferPointer(),
    root_sign_blob->GetBufferSize(), IID_PPV_ARGS(&root_sign));
  REI_ASSERT(SUCCEEDED(hr));
}

ID3D12GraphicsCommandList4* DeviceResources::prepare_command_list(ID3D12PipelineState* init_pso) {
  if (!is_using_cmd_list) {
    HRESULT hr = m_command_list->Reset(m_command_alloc.Get(), nullptr);
    REI_ASSERT((SUCCEEDED(hr)));
    is_using_cmd_list = true;
  }
  return m_command_list.Get();
}

void DeviceResources::flush_command_list() {
  // TODO check is not empty
  REI_ASSERT(is_using_cmd_list);
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
