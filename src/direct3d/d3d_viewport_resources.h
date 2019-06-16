#ifndef REI_D3D_VIEWPORT_RESOURCES_H
#define REI_D3D_VIEWPORT_RESOURCES_H

#include <windows.h>
#include <d3d12.h>
#include <d3d11_2.h>

#include "d3d_device_resources.h"

namespace rei {

class D3DViewportResources {
public:
  D3DViewportResources() = delete;
  D3DViewportResources(ID3D11Device* device, HWND hwnd, int init_width, int init_heigh);
  ~D3DViewportResources();

  void update_size(int width, int height);

private:
  friend D3DDeviceResources;
  friend D3DRenderer;

  int width;
  int height;

  HWND hwnd;
  ID3D11Device* d3d11Device;                // the device abstraction
  ID3D11DeviceContext* d3d11DevCon;         // the device context

  IDXGISwapChain1* SwapChain;                // double-buffering
  ID3D11Texture2D* depthStencilBuffer;      // texture buffer for depth test
  ID3D11RenderTargetView* renderTargetView; // render target interface
  ID3D11DepthStencilView* depthStencilView; // depth-testing interface

  void create_size_dependent_resources();

};

}

#endif
