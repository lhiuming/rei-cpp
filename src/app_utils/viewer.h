#ifndef REI_VIEWER_H
#define REI_VIEWER_H

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <windows.h>

#include "console.h"
#include "input.h"
#include "renderer.h"

/**
 * viewer.h
 * Viewer class mange a window and setting interactive input by user using
 * Direct3D API. 
 */

namespace rei {

class WinViewer {
public:
  WinViewer() = delete;
  WinViewer(HINSTANCE h_instance, std::size_t window_w, std::size_t window_h, std::wstring title);
  ~WinViewer();

  SystemWindowID get_window_id() const { return WindowID(m_hwnd); }
  std::size_t width() const { return m_client_width; }
  std::size_t height() const { return m_client_height; }
  const std::wstring& title() const { return m_title; }

  bool is_destroyed() const { return m_is_destroyed; }

  void set_input_bus(std::weak_ptr<InputBus> input_bus);
  void update_title(const std::wstring& title);

private:
  struct WindowID : SystemWindowID {
    WindowID(HWND hwnd) : SystemWindowID {Platform::Win, hwnd} {}
  };

  // Configuration
  bool m_windowed = true;
  bool m_post_quit_on_destroy_msg = true;

  // Window states
  std::wstring m_title = L"No Title";
  std::size_t m_client_width, m_client_height;
  std::size_t m_window_width, m_window_height;
  bool m_is_destroyed = false;

  // Windows interface object
  const HINSTANCE m_hinstance = NULL;
  LPCWSTR m_wnd_class_name = L"rei_Viewer_Window";
  HWND m_hwnd = NULL;

  // Some transient input state
  bool mouse_in_window = false;
  POINTS last_mouse_pos_regular;

  // Input bus object
  std::weak_ptr<InputBus> input_bus;

  void initialize_window();
  void on_window_destroy();
  void update_cursor_range(InputBus* input);

  // Converto to basis with origin at Left-Bottom
  inline POINTS regularize(POINTS p) { return POINTS {p.x, SHORT(m_client_height) - p.y}; }

  LRESULT process_wnd_msg(UINT msg, WPARAM wParam, LPARAM lParam);

  struct HwndHasher {
    size_t operator()(const HWND& hwnd) const { return (std::uintptr_t)(hwnd); }
  };

  // Map hwnd to viewer class, for procceessing msg with context
  static std::unordered_map<HWND, WinViewer*, HwndHasher> viewer_map;

  // Windows static callback; telecast the mes to viewer
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

using Viewer = WinViewer;

} // namespace rei

#endif
