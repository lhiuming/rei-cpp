#if DIRECT3D_ENABLED

#include "../common.h"
#include "d3d_viewport_resources.h"
#include <dxgi1_2.h>

using std::runtime_error;

namespace rei {

D3DViewportResources::D3DViewportResources(
  ComPtr<ID3D12Device> device, HWND hwnd, int init_width, int init_heigh)
    : device(device), hwnd(hwnd), width(width), height(height), double_buffering(true) {
  create_size_dependent_resources();
}

D3DViewportResources::~D3DViewportResources() {
  // Release D3D interfaces
  SwapChain->Release();
  renderTargetView->Release();
  depthStencilView->Release();
  depthStencilBuffer->Release();
}

void D3DViewportResources::create_size_dependent_resources() {
  HRESULT hr;

  // Create swapchain, using the new api recommded by MS

  ComPtr<IDXGIDevice2> dxgi_device;
  ComPtr<IDXGIAdapter> dxgi_adapter;
  ComPtr<IDXGIFactory2> dxgi_factory;
  ASSERT(SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&dxgi_device))));
  ASSERT(SUCCEEDED(dxgi_device->GetParent(IID_PPV_ARGS(&dxgi_adapter))));
  ASSERT(SUCCEEDED(dxgi_factory->GetParent(IID_PPV_ARGS(&dxgi_device))));

  DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc; // use it if you need fullscreen
  fullscreen_desc.RefreshRate.Numerator = 60;
  fullscreen_desc.RefreshRate.Denominator = 1;
  fullscreen_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  fullscreen_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  DXGI_SWAP_CHAIN_DESC1 chain_desc;
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
  hr = dxgi_factory->CreateSwapChainForHwnd(device.Get(), hwnd, &chain_desc,
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

void D3DViewportResources::update_size(int width, int height) {
  this->width = width;
  this->height = height;
  create_size_dependent_resources();
}

}

#endif
