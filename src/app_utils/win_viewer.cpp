// Source of d3d_viewer.h
#include "win_viewer.h"

#include "../console.h" // reporting

#include <iostream>
#include <stdexcept>

using std::runtime_error;

namespace rei {

std::unordered_map<HWND, WinViewer*, WinViewer::HwndHasher> WinViewer::viewer_map = {};

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

  // Un-register from the global mao
  viewer_map.erase(hwnd);

  console << "D3DViewer is destructed." << std::endl;
}

void WinViewer::initialize_window(
  HINSTANCE hInstance, int ShowWnd, int width, int height, bool windowed) {
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
    m_title.c_str(),                // used in the window title bar (if any)
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

  // register to the global map
  viewer_map[hwnd] = this;

  ShowWindow(this->hwnd, ShowWnd); // show it anyway
  UpdateWindow(this->hwnd);        // paint the client area
}

LRESULT WinViewer::process_wnd_msg(UINT msg, WPARAM wParam, LPARAM lParam) {
  // Internal input handeling
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

  // Some user-define input event
  if (!input_bus.expired()) {
    InputBus& input = (*input_bus.lock());
    switch (msg) {
      case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) { // press ESC
          DestroyWindow(hwnd);
        }
        return 0;
      case WM_MOUSEMOVE:
        POINTS p1 = regularize(MAKEPOINTS(lParam));
        if (mouse_in_window) {
          POINTS p0 = last_mouse_pos_regular;
          input.push(CursorMove(p0.x, p0.y, p1.x, p1.y));
          if (wParam == MK_LBUTTON) { input.push(CursorDrag(p0.x, p0.y, p1.x, p1.y)); }
        }
        last_mouse_pos_regular = p1;
        mouse_in_window = true;
        return 0;
      case WM_MOUSEWHEEL:
        double delta = GET_WHEEL_DELTA_WPARAM(wParam) / (double)WHEEL_DELTA;
        bool altered = (GET_KEYSTATE_WPARAM(wParam) == MK_SHIFT);
        input.push(Zoom {delta, altered});
        return 0;
    }
  }

  // Default case: use windows' default procudure
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// Windows message callback handler
LRESULT CALLBACK WinViewer::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  auto viewer = viewer_map.find(hwnd);
  if (viewer != viewer_map.end()) { return viewer->second->process_wnd_msg(msg, wParam, lParam); }
  ERROR("Fail to find viewer");
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void WinViewer::update_title(const std::wstring& new_title) {
  BOOL succeeded = SetWindowTextW(hwnd, new_title.data());
  ASSERT(succeeded);
  m_title = new_title;
}

} // namespace rei