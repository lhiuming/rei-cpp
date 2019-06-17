#ifndef REI_DIRECT3D_D3D_VIEWER_H
#define REI_DIRECT3D_D3D_VIEWER_H

#include <cstddef>

#include <string>
#include <vector>

#include <windows.h>
#include <d3d11.h>

#include "../common.h"
#include "../camera.h"
#include "../scene.h"
#include "../renderer.h"
#include "viewer.h"
//#include "d3d_renderer.h" // win32 renderer


/*
 * d3d_viewer.h
 * Viewer class mange a window and setting interactive input by user using
 * Direct3D API. It also configure a renderer and provide it with necessary
 * infomation to render the window.
 */

namespace rei {

class WinViewer : public Viewer {
public:

  WinViewer() = delete;
  WinViewer(HINSTANCE h_instance, std::size_t window_w, std::size_t window_h, std::wstring title);
  ~WinViewer();

  void init_viewport(Renderer& renderer) override {
    UNINIT(viewport) = renderer.create_viewport(WindowID(hwnd), width, height);
  }

  void update_title(const std::wstring& title) override;

private:
  struct WindowID : SystemWindowID {
    WindowID(HWND hwnd) : SystemWindowID {Win, hwnd} {}
  };

  // Windows interface object
  HINSTANCE h_instance;
  LPCWSTR WndClassName = L"rei_Viewer_Window";
  HWND hwnd;

  void initialize_window(HINSTANCE hInstance, int ShowWnd, int width, int height, bool windowed);

  // Window callback; useful for setting key-binding
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

}

#endif
