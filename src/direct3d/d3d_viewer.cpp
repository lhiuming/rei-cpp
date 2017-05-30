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
  // TODO

  // Some data normally get from WinMain
  HINSTANCE hInstance = GetModuleHandle(nullptr); // handle to the current .exe 

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
  hwnd = CreateWindowEx(
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
  if (!hwnd)
  {
    MessageBox(NULL, "Error creating window", "error", MB_OK | MB_ICONERROR);
    throw runtime_error("D3DViewer Construction Failed");
  }

  ShowWindow(hwnd, SW_SHOW); // show it anyway 
  UpdateWindow(hwnd); // paint the client area

}


// Destructor 
D3DViewer::~D3DViewer()
{
  // TODO

  DestroyWindow(hwnd);
}


// Update&render loop
void D3DViewer::run()
{
  // TODO 

  // Make sure the renderer is set properly 

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
    {
      // run game code            
      //scene->update();
      //render->draw();
    }
  }  // end while 

}


} // namespace CEL