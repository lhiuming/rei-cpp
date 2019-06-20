#ifndef REI_D3D_VIEWPORT_RESOURCES_H
#define REI_D3D_VIEWPORT_RESOURCES_H

#include <windows.h>
#include <wrl.h>
#include <d3d11_2.h>
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

  void update_size(int width, int height);

private:
  friend DeviceResources;
  friend Renderer;

  int width;
  int height;

  RenderTargetSpec target_spec;

  bool double_buffering;

  HWND hwnd;
  ComPtr<ID3D12Device> device;
  ComPtr<IDXGIFactory4> dxgi_factory;

  ComPtr<ID3D12CommandQueue> command_queue;
  ComPtr<IDXGISwapChain1> swapchain;

  ID3D11Device* d3d11Device;        // the device abstraction
  ID3D11DeviceContext* d3d11DevCon; // the device context

  IDXGISwapChain1* SwapChain;               // double-buffering
  ID3D11Texture2D* depthStencilBuffer;      // texture buffer for depth test
  ID3D11RenderTargetView* renderTargetView; // render target interface
  ID3D11DepthStencilView* depthStencilView; // depth-testing interface

  void create_size_dependent_resources();
};

} // namespace d3d

} // namespace rei

#endif
