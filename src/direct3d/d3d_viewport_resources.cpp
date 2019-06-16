#if DIRECT3D_ENABLED

#include "../common.h"
#include "d3d_viewport_resources.h"
#include <dxgi1_2.h>

using std::runtime_error;

namespace rei {

D3DViewportResources::D3DViewportResources(ID3D11Device* device, HWND hwnd, int width, int height)
    : d3d11Device(device), hwnd(hwnd), width(width), height(height) {
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

  IDXGIDevice2* pDXGIDevice;
  hr = d3d11Device->QueryInterface(__uuidof(IDXGIDevice2), (void**)&pDXGIDevice);
  ASSERT(SUCCEEDED(hr));
  IDXGIAdapter* pDXGIAdapter;
  hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&pDXGIAdapter);
  ASSERT(SUCCEEDED(hr));
  IDXGIFactory2* pIDXGIFactory;
  hr = pDXGIAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&pIDXGIFactory);
  ASSERT(SUCCEEDED(hr));
  
  DXGI_SWAP_CHAIN_FULLSCREEN_DESC swap_chain_fs_desc; // use it if you need full screen 
  ZeroMemory(&swap_chain_fs_desc, sizeof(DXGI_SWAP_CHAIN_FULLSCREEN_DESC));
  swap_chain_fs_desc.RefreshRate.Numerator = 60;
  swap_chain_fs_desc.RefreshRate.Denominator = 1;
  swap_chain_fs_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  swap_chain_fs_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  DXGI_SWAP_CHAIN_DESC1 swapChainDesc; // swap-chain property
  ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
  swapChainDesc.Width = width;
  swapChainDesc.Height = height;
  swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.Stereo = FALSE;
  swapChainDesc.SampleDesc.Count = 1; // multi-sampling parameters; should match with effetive buffer size
  swapChainDesc.SampleDesc.Quality = 0;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.BufferCount = 1; // 1 for double buffer; 2 for triple, and so on
  swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
  hr = pIDXGIFactory->CreateSwapChainForHwnd(d3d11Device,
    hwnd,
    &swapChainDesc,
    NULL, //&swap_chain_fs_desc,
    NULL, // optional
    &(this->SwapChain)   // receive the returned swap-chain pointer
  );
  if (FAILED(hr)) { throw runtime_error("Device creation FAILED"); }

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
