#include "viewer.h"

#include <iostream>
#include <stdexcept>

#include "console.h" // reporting

using std::runtime_error;

namespace rei {

std::unordered_map<HWND, WinViewer*, WinViewer::HwndHasher> WinViewer::viewer_map = {};

// Constructor
WinViewer::WinViewer(HINSTANCE h_instance, size_t width, size_t height, std::wstring title)
    : m_client_width(width),
      m_client_height(height),
      m_window_width(width),
      m_window_height(height),
      m_title(title) {
  // Initialize a window
  bool show_startup = true;
  initialize_window();
}

// Destructor
WinViewer::~WinViewer() {
  if (!m_is_destroyed) {
    // Destroy the window
    DestroyWindow(m_hwnd);
    // Un-register from the global mao
    viewer_map.erase(m_hwnd);
  }
}

void WinViewer::initialize_window() {
  // Register
  WNDCLASSEXW wc_prop;
  wc_prop.cbSize = sizeof(WNDCLASSEXW);
  wc_prop.style = CS_HREDRAW | CS_VREDRAW;
  wc_prop.lpfnWndProc = WndProc; // class static member function
  wc_prop.cbClsExtra = NULL;
  wc_prop.cbWndExtra = NULL;
  wc_prop.hInstance = m_hinstance;
  wc_prop.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc_prop.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc_prop.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
  wc_prop.lpszMenuName = NULL;
  wc_prop.lpszClassName = m_wnd_class_name; // class member
  wc_prop.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
  if (!RegisterClassExW(&wc_prop)) {
    MessageBox(NULL, "Error registering window", "Error", MB_OK | MB_ICONERROR);
    throw runtime_error("D3DViewer Construction Failed");
  }

  // Create the window
  const DWORD wnd_style = WS_OVERLAPPEDWINDOW;
  RECT rect {0, 0, m_client_width, m_client_height};
  bool adjusted = ::AdjustWindowRect(&rect, wnd_style, false);
  REI_ASSERT(adjusted);
  m_window_width = rect.right - rect.left;
  m_window_height = rect.bottom - rect.top;
  m_hwnd = CreateWindowExW(0,        // choose an extended window style; 0 is default
    m_wnd_class_name,                // optional; a registered class name
    m_title.c_str(),                 // used in the window title bar (if any)
    wnd_style,                       // window style; using the most ordinary combination
    CW_USEDEFAULT, CW_USEDEFAULT,    // window default position, see MSDN
    m_window_width, m_window_height, // window size
    NULL,                            // optional; handle to the parent/owner window
    NULL,                            // optional; handle to a menu or child-window
    m_hinstance,                     // optional; handle to module instance to be associated
    NULL                             // optional; some special usage, see MSDN
  );
  if (!m_hwnd) {
    MessageBox(NULL, "Error creating window", "error", MB_OK | MB_ICONERROR);
    throw runtime_error("D3DViewer Construction Failed");
  }

  // register to the global map
  viewer_map[m_hwnd] = this;
  m_is_destroyed = false;

  ShowWindow(m_hwnd, m_windowed); // show it anyway
  UpdateWindow(m_hwnd);           // paint the client area
}

void WinViewer::on_window_destroy() {
  viewer_map.erase(m_hwnd);
  m_is_destroyed = true;
  m_hwnd = NULL;
}

LRESULT WinViewer::process_wnd_msg(UINT msg, WPARAM wParam, LPARAM lParam) {
  // Internal input handeling
  switch (msg) {
    case WM_DESTROY: // click (X)
      if (m_post_quit_on_destroy_msg) { PostQuitMessage(0); }
      on_window_destroy();
      return 0;
    case WM_PAINT:
      return 0;
    case WM_KEYDOWN: {
      if (wParam == VK_ESCAPE) { // press ESC
        DestroyWindow(m_hwnd);
      }
      return 0;
    }
    case WM_SIZE: {
      m_window_height = LOWORD(lParam);
      m_window_width = HIWORD(lParam);
      RECT rect;
      bool got = ::GetClientRect(m_hwnd, &rect);
      REI_ASSERT(got);
      m_client_width = rect.right - rect.left;
      m_client_height = rect.bottom - rect.top;
      update_cursor_range(input_bus.lock().get());
      return 0;
    }
  }

  // Some user-define input event
  if (!input_bus.expired()) {
    InputBus& input = (*input_bus.lock());
    switch (msg) {
      case WM_LBUTTONUP:
      case WM_MBUTTONUP:
      case WM_RBUTTONUP:
        input.push(CursorUp());
        return 0;
      case WM_LBUTTONDOWN:
      case WM_MBUTTONDOWN:
      case WM_RBUTTONDOWN:
        input.push(CursorDown());
        return 0;
      case WM_MOUSEMOVE: {
        POINTS p1 = regularize(MAKEPOINTS(lParam));
        if (mouse_in_window) {
          POINTS p0 = last_mouse_pos_regular;
          input.push(CursorMove(p0.x, p0.y, p1.x, p1.y));
          if (wParam == MK_LBUTTON) {
            input.push(CursorDrag(p0.x, p0.y, p1.x, p1.y, CursorAlterType::Left));
          }
          if (wParam == MK_RBUTTON) {
            input.push(CursorDrag(p0.x, p0.y, p1.x, p1.y, CursorAlterType::Right));
          }
          if (wParam == MK_MBUTTON) {
            input.push(CursorDrag(p0.x, p0.y, p1.x, p1.y, CursorAlterType::Middle));
          }
        }
        last_mouse_pos_regular = p1;
        mouse_in_window = true;
        return 0;
      }
      case WM_MOUSEWHEEL: {
        double delta = GET_WHEEL_DELTA_WPARAM(wParam) / (double)WHEEL_DELTA;
        bool altered = (GET_KEYSTATE_WPARAM(wParam) == MK_SHIFT);
        input.push(Zoom {delta, altered});
        return 0;
      }
    }
  }

  // Default case: use windows' default procudure
  return DefWindowProcW(m_hwnd, msg, wParam, lParam);
}

// Windows message callback handler
LRESULT CALLBACK WinViewer::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  auto viewer = viewer_map.find(hwnd);
  if (viewer != viewer_map.end()) { return viewer->second->process_wnd_msg(msg, wParam, lParam); }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void WinViewer::set_input_bus(std::weak_ptr<InputBus> input_bus) {
  this->input_bus = input_bus;
  update_cursor_range(input_bus.lock().get());
}

void WinViewer::update_title(const std::wstring& new_title) {
  if (m_is_destroyed) {
    REI_WARNING("Window is already destroyed");
    return;
  }
  BOOL succeeded = SetWindowTextW(m_hwnd, new_title.data());
  REI_ASSERT(succeeded);
  m_title = new_title;
}

void WinViewer::update_cursor_range(InputBus* input) {
  if (input) {
    Vec3 left_top {0, 0, 0};
    Vec3 right_bottom {double(m_client_width), double(m_client_height), 0.0f};
    input->set_cursor_range(left_top, right_bottom);
  }
}

} // namespace rei