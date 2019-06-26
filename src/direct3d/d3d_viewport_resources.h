#ifndef REI_D3D_VIEWPORT_RESOURCES_H
#define REI_D3D_VIEWPORT_RESOURCES_H

#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#include "d3d_common_resources.h"
#include "d3d_device_resources.h"

namespace rei {

namespace d3d {
  
using Microsoft::WRL::ComPtr;

class Renderer;

class ViewportResources {
public:
  ViewportResources() = delete;
  ViewportResources(ComPtr<ID3D12Device> device, ComPtr<IDXGIFactory4> dxgi_factory,
    ComPtr<ID3D12CommandQueue> command_queue, HWND hwnd, int init_width, int init_height);
  ~ViewportResources();

private:
  friend Renderer;

  int width;
  int height;
  const UINT swapchain_buffer_count;
  const RenderTargetSpec target_spec;

  HWND hwnd;

  ComPtr<ID3D12Device> device;
  ComPtr<IDXGIFactory4> dxgi_factory;
  ComPtr<ID3D12CommandQueue> command_queue;

  ComPtr<ID3D12DescriptorHeap> rtv_heap;
  ComPtr<ID3D12DescriptorHeap> dsv_heap;
  ComPtr<IDXGISwapChain1> swapchain;
  ComPtr<ID3D12Resource> depth_stencil_buffer;

  UINT64 rtv_descriptor_size = -1;
  UINT current_back_buffer_index = 0;

  void update_size(int width, int height);
  void create_size_dependent_resources();

  ComPtr<ID3D12Resource> get_current_rt_buffer() const {
    return get_rt_buffer(current_back_buffer_index);
  }
  ComPtr<ID3D12Resource> get_rt_buffer(UINT index) const;
  D3D12_CPU_DESCRIPTOR_HANDLE get_current_rtv() const {
    return get_rtv(current_back_buffer_index);
  }
  D3D12_CPU_DESCRIPTOR_HANDLE get_rtv(UINT offset) const;
  ComPtr<ID3D12Resource> get_ds_buffer() const { return depth_stencil_buffer; }
  D3D12_CPU_DESCRIPTOR_HANDLE get_dsv() const;

  void flip_backbuffer() {
    current_back_buffer_index = (current_back_buffer_index + 1) % swapchain_buffer_count;
  }

};

} // namespace d3d

} // namespace rei

#endif
