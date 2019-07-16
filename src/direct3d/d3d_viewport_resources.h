#ifndef REI_D3D_VIEWPORT_RESOURCES_H
#define REI_D3D_VIEWPORT_RESOURCES_H

#include <d3d12.h>
#include <dxgi1_4.h>
#include <windows.h>
#include <wrl.h>

#include "d3d_common_resources.h"
#include "d3d_device_resources.h"
#include "d3d_utils.h"

namespace rei {

namespace d3d {

using Microsoft::WRL::ComPtr;

class ViewportResources {
public:
  ViewportResources() = delete;
  ViewportResources(
    std::shared_ptr<DeviceResources> device_resources, HWND hwnd, int init_width, int init_height, v_array<std::shared_ptr<DefaultBufferData>, 4> rt_buffers, std::shared_ptr<DefaultBufferData> ds_buffer);
  ~ViewportResources();

  ID3D12DescriptorHeap* rtv_heap() const { return m_rtv_heap.Get(); }
  ID3D12DescriptorHeap* dsv_heap() const { return m_dsv_heap.Get(); }
  IDXGISwapChain1* swapchain() const { return m_swapchain.Get(); }

  void update_size(int width, int height);

  const RenderTargetSpec& target_spec() const { return m_target_spec; }

  ID3D12Resource* get_current_rt_buffer() const { return get_rt_buffer(current_back_buffer_index); }
  ID3D12Resource* get_rt_buffer(UINT index) const;
  UINT get_current_rt_index() const { return current_back_buffer_index; }
  D3D12_CPU_DESCRIPTOR_HANDLE get_current_rtv() const { return get_rtv(current_back_buffer_index); }
  D3D12_CPU_DESCRIPTOR_HANDLE get_rtv(UINT offset) const;
  ID3D12Resource* get_ds_buffer() const { return m_ds_buffer->buffer.Get(); }
  D3D12_CPU_DESCRIPTOR_HANDLE get_dsv() const;

  void flip_backbuffer() {
    current_back_buffer_index = (current_back_buffer_index + 1) % swapchain_buffer_count;
  }

  // temporarily
  const v_array<std::shared_ptr<DefaultBufferData>, 4> m_rt_buffers;
  const std::shared_ptr<DefaultBufferData> m_ds_buffer;

protected:
  ID3D12Device* device() const { return m_device_resources->device(); }
  IDXGIFactory4* dxgi_factory() const { return m_device_resources->dxgi_factory(); }
  ID3D12CommandQueue* command_queue() const { return m_device_resources->command_queue(); }

  void create_size_dependent_resources();

private:
  int width;
  int height;
  const UINT swapchain_buffer_count;
  const RenderTargetSpec m_target_spec;

  HWND hwnd;
  std::shared_ptr<DeviceResources> m_device_resources;

  // Swapchain objects
  ComPtr<ID3D12DescriptorHeap> m_rtv_heap;
  ComPtr<ID3D12DescriptorHeap> m_dsv_heap;
  ComPtr<IDXGISwapChain3> m_swapchain;

  UINT64 rtv_descriptor_size = -1;
  UINT current_back_buffer_index = 0;
};

} // namespace d3d

} // namespace rei

#endif
