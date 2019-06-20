#if DIRECT3D_ENABLED
#include "d3d_viewport_resources.h"

#include "../common.h"

using std::runtime_error;

namespace rei {

namespace d3d {

ViewportResources::ViewportResources(ComPtr<ID3D12Device> device,
  ComPtr<IDXGIFactory4> dxgi_factory,
  ComPtr<ID3D12CommandQueue> command_queue, HWND hwnd, int init_width, int init_height)
    : device(device),
      command_queue(command_queue),
      dxgi_factory(dxgi_factory),
      hwnd(hwnd),
      width(init_width),
      height(init_height),
      double_buffering(true) {
  ASSERT(device);
  ASSERT(dxgi_factory);
  ASSERT(command_queue);
  ASSERT(hwnd);
  ASSERT((width > 0) && (height > 0));
  create_size_dependent_resources();
}

ViewportResources::~ViewportResources() {
  // Release D3D interfaces
  SwapChain->Release();
  renderTargetView->Release();
  depthStencilView->Release();
  depthStencilBuffer->Release();
}

void ViewportResources::create_size_dependent_resources() {
  HRESULT hr;

  /*
   * NOTE: In DX12, IDXGIDevice is not available anymore, so IDXGIFactory/IDXGIAdaptor has to be created/specified, rather than being found through IDXGIDevice.
   */

  DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc = {}; // use it if you need fullscreen
  fullscreen_desc.RefreshRate.Numerator = 60;
  fullscreen_desc.RefreshRate.Denominator = 1;
  fullscreen_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  fullscreen_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  DXGI_SWAP_CHAIN_DESC1 chain_desc = {};
  chain_desc.Width = width;
  chain_desc.Height = height;
  chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  chain_desc.Stereo = FALSE;
  chain_desc.SampleDesc.Count = 1; // multi-sampling parameters;
  chain_desc.SampleDesc.Quality = 0;
  chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  chain_desc.BufferCount = double_buffering ? 2 : 1; // total frame buffer count
  chain_desc.Scaling = DXGI_SCALING_STRETCH;
  chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // TODO DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
  chain_desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
  chain_desc.Flags = 0;
  hr = dxgi_factory->CreateSwapChainForHwnd(command_queue.Get(), hwnd, &chain_desc,
    NULL, // windowed app
    NULL, // optional
    &swapchain);
  ASSERT(SUCCEEDED(hr));

  return;

  // Render Target Interface (bound to the OutputMerge stage)
  ID3D11Texture2D* BackBuffer; // tmp pointer to the backbuffer in swap-chain
  hr = this->SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);
  if (FAILED(hr)) throw runtime_error("BackBuffer creation FAILED");
  hr = this->d3d11Device->CreateRenderTargetView(BackBuffer, // resource for the view
    NULL,                                                    // optional; render arget desc
    &(this->renderTargetView) // receive the returned render target view
  );
  if (FAILED(hr)) throw runtime_error("RenderTargetView creation FAILED");
  BackBuffer->Release();

  // Depth Buffer Interface
  D3D11_TEXTURE2D_DESC depthStencilDesc; // property for the depth buffer
  depthStencilDesc.Width = width;
  depthStencilDesc.Height = height;
  depthStencilDesc.MipLevels = 1;                          // max number of mipmap level;
  depthStencilDesc.ArraySize = 1;                          // number of textures
  depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 24 bit for depth
  depthStencilDesc.SampleDesc.Count = 1;                   // multi-sampling parameters
  depthStencilDesc.SampleDesc.Quality = 0;
  depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
  depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
  depthStencilDesc.CPUAccessFlags = 0; // we dont access it
  depthStencilDesc.MiscFlags = 0;      // additional settings
  hr = this->d3d11Device->CreateTexture2D(&depthStencilDesc,
    NULL,                       // optinoal; the data to send to the buffer; we don't set it ourself
    &(this->depthStencilBuffer) // receive the returned buffer pointer
  );
  if (FAILED(hr)) throw runtime_error("DepthBuffer creation FAILED");
  hr = this->d3d11Device->CreateDepthStencilView(this->depthStencilBuffer, // resource for the view
    NULL,                     // optional; pointer to depth-stencil-view desc; NULL to use 0-mipmap
    &(this->depthStencilView) // receive the returned DepthStencil View pointer
  );
  if (FAILED(hr)) throw runtime_error("DepthStencilView creation FAILED");
}

void ViewportResources::update_size(int width, int height) {
  this->width = width;
  this->height = height;
  create_size_dependent_resources();
}

} // namespace d3d

} // namespace rei

#endif
