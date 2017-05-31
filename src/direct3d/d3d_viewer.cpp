// Source of d3d_viewer.h
#include "d3d_viewer.h"

#include "../console.h" // reporting 

#include <stdexcept>
#include <iostream>

using namespace std;

namespace CEL {

// Windows message callback handler 
LRESULT CALLBACK 
D3DViewer::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  // Choose a reaction for keyboard or mouse events 
  switch (msg)
  {
  case WM_KEYDOWN:
    if (wParam == VK_ESCAPE) {  // press ESC
      DestroyWindow(hwnd);
    }
    return 0;

  case WM_DESTROY:  // click (X)
    PostQuitMessage(0);
    return 0;
  }

  // Default case: use windows' default procudure  
  return DefWindowProc(hwnd, msg, wParam, lParam);
}


// Constructor 
D3DViewer::D3DViewer(size_t window_w, size_t window_h, string title)
{

  // Some data normally get from WinMain
  HINSTANCE hInstance = GetModuleHandle(nullptr); // handle to the current .exe 
  this->width = window_w;
  this->height = window_h;


  // Initialize a window (Extended Window) // 

  // Register 
  WNDCLASSEX wc_prop;
  wc_prop.cbSize = sizeof(WNDCLASSEX);
  wc_prop.style = CS_HREDRAW | CS_VREDRAW;
  wc_prop.lpfnWndProc = WndProc; // class static member function 
  wc_prop.cbClsExtra = NULL;
  wc_prop.cbWndExtra = NULL;
  wc_prop.hInstance = hInstance;  // got above 
  wc_prop.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc_prop.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc_prop.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
  wc_prop.lpszMenuName = NULL;
  wc_prop.lpszClassName = WndClassName;  // class member 
  wc_prop.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
  if (!RegisterClassEx(&wc_prop))
  {
    MessageBox(NULL, "Error registering window", 
      "Error", MB_OK | MB_ICONERROR);
    throw runtime_error("D3DViewer Construction Failed");
  }

  // Create the window 
  this->hwnd = CreateWindowEx(
    0, // choose an extended window style; 0 is default 
    WndClassName, // optional; a registered class name
    title.c_str(), // used in the window title bar (if any)
    WS_OVERLAPPEDWINDOW, // window style; using the most ordinary combination
    CW_USEDEFAULT, CW_USEDEFAULT, // window default position, see MSDN
    window_w, window_h, // window size 
    NULL, // optional; handle to the parent/owner window 
    NULL, // optional; handle to a menu or child-window 
    hInstance, // optional; handle to module instance to be associated 
    NULL // optional; some special usage, see MSDN
  );
  if (!(this->hwnd))
  {
    MessageBox(NULL, "Error creating window", "error", MB_OK | MB_ICONERROR);
    throw runtime_error("D3DViewer Construction Failed");
  }

  ShowWindow(this->hwnd, SW_SHOW); // show it anyway 
  UpdateWindow(this->hwnd); // paint the client area


  // Initialize D3D Context // 

  // Swap Chain and Device 
  DXGI_MODE_DESC bufferDesc;  // Buffer property for the swap-chain
  ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));
  bufferDesc.Width = window_w;
  bufferDesc.Height = window_h;
  bufferDesc.RefreshRate.Numerator = 60;
  bufferDesc.RefreshRate.Denominator = 1;
  bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  DXGI_SWAP_CHAIN_DESC swapChainDesc;  // swap-chain property 
  ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
  swapChainDesc.BufferDesc = bufferDesc;
  swapChainDesc.SampleDesc.Count = 1;  //multi-sampling parameters 
  swapChainDesc.SampleDesc.Quality = 0;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.BufferCount = 1;  // 1 for double buffer; 2 for triple, and so on
  swapChainDesc.OutputWindow = this->hwnd;
  swapChainDesc.Windowed = TRUE;  // change if you want full screen 
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  swapChainDesc.Flags = NULL;  // additional settings  
  D3D11CreateDeviceAndSwapChain(
    NULL,  // optional; pointer to video adapter
    D3D_DRIVER_TYPE_HARDWARE,  // driver type to create 
    NULL,  // handle to software rasterizer DLL; used when above is *_SOFTWARE
    NULL,  // enbling runtime software layers (see MSDN)
    NULL,  // optional; pointer to array of feature levels
    NULL,  // optional; number of candidate feature levels (in array above)
    D3D11_SDK_VERSION,  // fixed for d3d11
    &swapChainDesc,  // optional parameter for swap chain; optional as duplicated?
    &(this->SwapChain),   // receive the returned swap-chain pointer 
    &(this->d3d11Device), // receive the returned d3d device pointer
    NULL, // receive the returned featured level that actually used 
    &(this->d3d11DevCon)  // receive the return d3d device context pointer 
  );

  // Render Target Interface (bound to the OutputMerge stage)
  ID3D11Texture2D* BackBuffer;  // tmp pointer to the backbuffer in swap-chain 
  this->SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);
  d3d11Device->CreateRenderTargetView(
    BackBuffer,  // resource for the view  
    NULL,  // optional; render arget desc 
    &(this->renderTargetView)  // receive the returned render target view
  );
  BackBuffer->Release();

  // Depth Buffer Interface
  D3D11_TEXTURE2D_DESC depthStencilDesc;  // property for the depth buffer 
  depthStencilDesc.Width = window_w;
  depthStencilDesc.Height = window_h;
  depthStencilDesc.MipLevels = 1;  // max number of mipmap level;
  depthStencilDesc.ArraySize = 1;  // number of textures 
  depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;  // 24 bit for depth 
  depthStencilDesc.SampleDesc.Count = 1;  // multi-sampling parameters 
  depthStencilDesc.SampleDesc.Quality = 0;
  depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
  depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL; 
  depthStencilDesc.CPUAccessFlags = 0;  // we dont access it 
  depthStencilDesc.MiscFlags = 0;  // additional settings 
  this->d3d11Device->CreateTexture2D( 
    &depthStencilDesc, 
    NULL,  // optinoal; the data to send to the buffer; we don't set it ourself 
    &(this->depthStencilBuffer)  // receive the returned buffer pointer 
  );
  this->d3d11Device->CreateDepthStencilView(
    this->depthStencilBuffer,  // resource for the view 
    NULL,  // optional; pointer to depth-stencil-view desc; NULL to use 0-mipmap
    &(this->depthStencilView)  // receive the returned DepthStencil View pointer
  );

  // Setup the view port (used in the Rasterizer Stage) 
  D3D11_VIEWPORT viewport;
  ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
  viewport.TopLeftX = 0;  // position of 
  viewport.TopLeftY = 0;  //  the top-left corner in the window.
  viewport.Width = static_cast<float>(window_w);
  viewport.Height = static_cast<float>(window_h);
  viewport.MinDepth = 0.0f; // set depth range; used for converting z-values to depth  
  viewport.MaxDepth = 1.0f; // furthest value 

                            // Set the Viewport (bind to the Raster Stage of he pipeline) 
  d3d11DevCon->RSSetViewports(1, &viewport);

  // Bind Render Target View and StencilDepth View to OM stage
  this->d3d11DevCon->OMSetRenderTargets(
    1, // number of render targets to bind
    &(this->renderTargetView),
    this->depthStencilView
  );

}


// Destructor 
D3DViewer::~D3DViewer()
{
  // Destroy the window 
  DestroyWindow(hwnd);

  // Release D3D interfaces 
  SwapChain->Release();
  d3d11Device->Release();
  d3d11DevCon->Release();
  renderTargetView->Release();
  depthStencilView->Release();
  depthStencilBuffer->Release();

  console << "D3DViewer is destructed." << endl;
}


// Update&render loop
void D3DViewer::run()
{
  // Make sure the renderer is set properly 
  D3DRenderer& d3dRenderer = dynamic_cast<D3DRenderer&>(*renderer);
  d3dRenderer.set_buffer_size(width, height);
  d3dRenderer.set_d3d_interface(d3d11Device, d3d11DevCon);
  d3dRenderer.set_scene(scene);
  d3dRenderer.set_camera(camera);

  // Check event and do update&render
  MSG msg;
  ZeroMemory(&msg, sizeof(MSG));
  while (true)
  {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) // passge message to WndProc
    {
      if (msg.message == WM_QUIT)
        break;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } 
    else 
    { // Run game code            

      //scene->update();

      // clear windows background (render target)
      float bgColor[4] = { 0.3f, 0.6f, 0.7f, 0.5f };
      d3d11DevCon->ClearRenderTargetView(renderTargetView, bgColor);

      // Also clear the depth buffer 
      // TODO

      d3dRenderer.render();

      // Flip the buffer 
      SwapChain->Present(0, 0);
    }
  }  // end while 

}


} // namespace CEL