#ifndef CEL_DIRECT3D_D3D_VIEWER_H
#define CEL_DIRECT3D_D3D_VIEWER_H

#include <cstddef>

#include <string>
#include <vector>

#include "../scene.h"
#include "../camera.h"
#include "../viewer.h" // the base class
#include "d3d_renderer.h" // win32 renderer 
#include "../win32/safe_windows.h"


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

  LPCTSTR WndClassName = "CEL_Viewer_Window";
  HWND hwnd = nullptr;

  // Implement Helpers // 

  // Window callback; useful for setting key-binding
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam);

};

} // namespace CEL


#endif
