#ifndef REI_VIEWER_H
#define REI_VIEWER_H

#include <cstddef>
#include <string>
#include <vector>
#include <memory>

#include "../console.h"
#include "../input.h"
#include "../renderer.h"

/**
 * viewer.h
 * Define the interface for viewer, which manage a window and handle user-
 * inputs.
 *
 * Implementation is platform-specific. (see opengl/viewer.h and d3d/viewer.h)
 */

namespace rei {

class Viewer {
public:
  Viewer(std::size_t window_w, std::size_t window_h, std::wstring title) 
    : m_width(window_w), m_height(window_h), m_title(title) {}
  virtual ~Viewer() {};

  virtual SystemWindowID get_window_id() const = 0;

  virtual void update_title(const std::wstring& title) = 0;
  void set_input_bus(std::weak_ptr<InputBus> input_bus) { this->input_bus = input_bus; }

  std::size_t width() const { return m_width; }
  std::size_t height() const { return m_height; }
  const std::wstring& title() const { return m_title; }

  virtual bool is_destroyed() const = 0;

protected:
  std::size_t m_width, m_height;
  std::wstring m_title = L"No Title";

  std::weak_ptr<InputBus> input_bus;
};

}

#endif
