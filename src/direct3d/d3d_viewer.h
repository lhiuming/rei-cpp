#ifndef CEL_DIRECT3D_D3D_VIEWER_H
#define CEL_DIRECT3D_D3D_VIEWER_H

#include <cstddef>

#include <string>
#include <vector>

#include "../scene.h"
#include "../camera.h"
#include "../viewer.h" // the base class
#include "d3d_renderer.h" // win32 renderer 

#include <windows.h>
#include <d3d11.h>

/*
* d3d_viewer.h
* Viewer class mange a window and setting interactive input by user using
* Direct3D API. It also configure a renderer and provide it with necessary
* infomation to render the window.
*/

namespace CEL {

class D3DViewer : public Viewer {

public:

  // Default counstructor : not allowed
  D3DViewer() = delete;

  // Initialize with window size and title
  D3DViewer(std::size_t window_w, std::size_t window_h, std::string title);

  // Destructor
  ~D3DViewer();

  // Start the update&render loop
  void run() override;

private:

  std::size_t width, height;
  std::string title = "No Title";

  // Windows interface object
  LPCTSTR WndClassName = "CEL_Viewer_Window";
  HWND hwnd;

  // D3D interface object 
  IDXGISwapChain* SwapChain;  // double-buffering 
  ID3D11Device* d3d11Device;  // the device abstraction 
  ID3D11DeviceContext* d3d11DevCon;  // the device context 
  ID3D11RenderTargetView* renderTargetView;  // render target interface 
  ID3D11DepthStencilView* depthStencilView;  // depth-testing interface
  ID3D11Texture2D* depthStencilBuffer;       // texture buffer for depth test


  // Implement Helpers // 

  void initialize_window(HINSTANCE hInstance, int ShowWnd, 
    int width, int height, bool windowed);
  void initialize_d3d_interface(HINSTANCE hInstance);


  // Window callback; useful for setting key-binding
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam);

};

} // namespace CEL


#endif
