#if DIRECT3D_ENABLED
#include "d3d_viewport_resources.h"

#include <memory>

#include "../common.h"

using std::runtime_error;
using std::shared_ptr;

namespace rei {

namespace d3d {

ViewportResources::ViewportResources(shared_ptr<DeviceResources> dev_res, HWND hwnd, int init_width, int init_height)
    : m_device_resources(dev_res),
      hwnd(hwnd),
      width(init_width),
      height(init_height),
      swapchain_buffer_count(2) {
  REI_ASSERT(m_device_resources);
  REI_ASSERT(hwnd);
  REI_ASSERT((width > 0) && (height > 0));

  HRESULT hr;

  // Create rtv and dsv heaps
  D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc;
  rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtv_heap_desc.NumDescriptors = swapchain_buffer_count;
  rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // TODO check this
  rtv_heap_desc.NodeMask = 0;                            // TODO check this
  hr = device().CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&m_rtv_heap));
  REI_ASSERT(SUCCEEDED(hr));

  current_back_buffer_index = 0;
  rtv_descriptor_size = device().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc;
  dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  dsv_heap_desc.NumDescriptors = 1; // afterall we need only one DS buffer
  dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  dsv_heap_desc.NodeMask = 0;
  hr = device().CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(&m_dsv_heap));
  REI_ASSERT(SUCCEEDED(hr));

  // Some size-dependent resources
  create_size_dependent_resources();
}

ViewportResources::~ViewportResources() {
  // default behaviour
}

ID3D12Resource* ViewportResources::get_rt_buffer(UINT index) const {
  REI_ASSERT(index <= swapchain_buffer_count);
  ComPtr<ID3D12Resource> ret;
  REI_ASSERT(SUCCEEDED(m_swapchain->GetBuffer(index, IID_PPV_ARGS(&ret))));
  return ret.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE ViewportResources::get_rtv(UINT index) const {
  return CD3DX12_CPU_DESCRIPTOR_HANDLE(
    m_rtv_heap->GetCPUDescriptorHandleForHeapStart(), index, rtv_descriptor_size);
}

D3D12_CPU_DESCRIPTOR_HANDLE ViewportResources::get_dsv() const {
  return m_dsv_heap->GetCPUDescriptorHandleForHeapStart();
}

void ViewportResources::create_size_dependent_resources() {
  HRESULT hr;

  // Create Swapchain
  /*
   * NOTE: In DX12, IDXGIDevice is not available anymore, so IDXGIFactory/IDXGIAdaptor has to be
   * created/specified, rather than being found through IDXGIDevice.
   */
  DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc = {}; // use it if you need fullscreen
  fullscreen_desc.RefreshRate.Numerator = 60;
  fullscreen_desc.RefreshRate.Denominator = 1;
  fullscreen_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  fullscreen_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  DXGI_SWAP_CHAIN_DESC1 chain_desc = {};
  chain_desc.Width = width;
  chain_desc.Height = height;
  chain_desc.Format = m_target_spec.rt_format;
  chain_desc.Stereo = FALSE;
  chain_desc.SampleDesc = m_target_spec.sample_desc;
  chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  chain_desc.BufferCount = swapchain_buffer_count; // total frame buffer count
  chain_desc.Scaling = DXGI_SCALING_STRETCH;
  chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // TODO DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
  chain_desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
  chain_desc.Flags = 0;
  hr = dxgi_factory().CreateSwapChainForHwnd(&command_queue(), hwnd, &chain_desc,
    NULL, // windowed app
    NULL, // optional
    &m_swapchain);
  REI_ASSERT(SUCCEEDED(hr));

  // Create render target views from swapchain buffers
  D3D12_RENDER_TARGET_VIEW_DESC* default_rtv_desc = nullptr; // default initialization
  for (UINT i = 0; i < swapchain_buffer_count; i++) {
    ComPtr<ID3D12Resource> rt_buffer;
    hr = m_swapchain->GetBuffer(i, IID_PPV_ARGS(&rt_buffer));
    REI_ASSERT(SUCCEEDED(hr));
    device().CreateRenderTargetView(rt_buffer.Get(), default_rtv_desc, get_rtv(i));
  }

  // Create depth stencil buffer
  D3D12_HEAP_PROPERTIES ds_heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  D3D12_HEAP_FLAGS ds_heap_flags = D3D12_HEAP_FLAG_NONE; // default
  D3D12_RESOURCE_DESC ds_desc = {};
  ds_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  ds_desc.Alignment = 0; // default; let the runtime choose
  ds_desc.Width = width;
  ds_desc.Height = height;
  ds_desc.DepthOrArraySize = 1;           // just a normal texture
  ds_desc.MipLevels = 1;                  // see above
  ds_desc.Format = m_target_spec.ds_format; // srandard choice
  ds_desc.SampleDesc = m_target_spec.sample_desc;
  ds_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;                       // defualt; TODO check this
  ds_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;             // as we intent
  D3D12_RESOURCE_STATES init_state = D3D12_RESOURCE_STATE_DEPTH_WRITE; // alway this state
  D3D12_CLEAR_VALUE optimized_clear = {}; // special clear value; usefull for framebuffer types
  optimized_clear.Format = m_target_spec.ds_format;
  optimized_clear.DepthStencil = m_target_spec.ds_clear;
  hr = device().CreateCommittedResource(&ds_heap_prop, ds_heap_flags, &ds_desc, init_state,
    &optimized_clear, IID_PPV_ARGS(&m_depth_stencil_buffer));
  REI_ASSERT(SUCCEEDED(hr));

  // Create depth-stencil view for the DS buffer
  D3D12_DEPTH_STENCIL_VIEW_DESC* p_dsv_desc = nullptr; // default value: same format as buffer[0]
  device().CreateDepthStencilView(m_depth_stencil_buffer.Get(), p_dsv_desc, get_dsv());
}

void ViewportResources::update_size(int width, int height) {
  this->width = width;
  this->height = height;
  create_size_dependent_resources();
}

} // namespace d3d

} // namespace rei

#endif
