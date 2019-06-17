// Source of d3d_viewer.h
#include "win_viewer.h"

#include "../console.h" // reporting

#include <iostream>
#include <stdexcept>

using std::runtime_error;

namespace rei {

// Constructor
WinViewer::WinViewer(HINSTANCE h_instance, size_t window_w, size_t window_h, std::wstring title)
    : Viewer(window_w, window_h, title) {
  // Initialize a window
  bool show_startup = true;
  initialize_window(h_instance, show_startup, window_w, window_h, true);
}

// Destructor
WinViewer::~WinViewer() {
  // Destroy the window
  DestroyWindow(hwnd);

  console << "D3DViewer is destructed." << std::endl;
}


void WinViewer::initialize_window(HINSTANCE hInstance, int ShowWnd, int width, int height, bool windowed) {
  // Register
  WNDCLASSEXW wc_prop;
  wc_prop.cbSize = sizeof(WNDCLASSEXW);
  wc_prop.style = CS_HREDRAW | CS_VREDRAW;
  wc_prop.lpfnWndProc = WndProc; // class static member function
  wc_prop.cbClsExtra = NULL;
  wc_prop.cbWndExtra = NULL;
  wc_prop.hInstance = hInstance;
  wc_prop.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc_prop.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc_prop.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
  wc_prop.lpszMenuName = NULL;
  wc_prop.lpszClassName = WndClassName; // class member
  wc_prop.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
  if (!RegisterClassExW(&wc_prop)) {
    MessageBox(NULL, "Error registering window", "Error", MB_OK | MB_ICONERROR);
    throw runtime_error("D3DViewer Construction Failed");
  }

  // Create the window
  hwnd = CreateWindowExW(0,       // choose an extended window style; 0 is default
    WndClassName,                 // optional; a registered class name
    title.c_str(),                // used in the window title bar (if any)
    WS_OVERLAPPEDWINDOW,          // window style; using the most ordinary combination
    CW_USEDEFAULT, CW_USEDEFAULT, // window default position, see MSDN
    width, height,                // window size
    NULL,                         // optional; handle to the parent/owner window
    NULL,                         // optional; handle to a menu or child-window
    hInstance,                    // optional; handle to module instance to be associated
    NULL                          // optional; some special usage, see MSDN
  );
  if (!(this->hwnd)) {
    MessageBox(NULL, "Error creating window", "error", MB_OK | MB_ICONERROR);
    throw runtime_error("D3DViewer Construction Failed");
  }

  ShowWindow(this->hwnd, ShowWnd); // show it anyway
  UpdateWindow(this->hwnd);        // paint the client area
}

// Windows message callback handler
LRESULT CALLBACK WinViewer::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  // Choose a reaction for keyboard or mouse events
  switch (msg) {
    case WM_KEYDOWN:
      if (wParam == VK_ESCAPE) { // press ESC
        DestroyWindow(hwnd);
      }
      return 0;

    case WM_DESTROY: // click (X)
      PostQuitMessage(0);
      return 0;
  }

  // Default case: use windows' default procudure
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void WinViewer::update_title(const std::wstring& new_title) {
  HRESULT hr = SetWindowTextW(hwnd, new_title.c_str());
  ASSERT(SUCCEEDED(hr));
}

}