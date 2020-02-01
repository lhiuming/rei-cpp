#include "viewport.h"

namespace rei {

Viewport::Viewport(std::weak_ptr<Renderer> renderer, SystemWindowID wnd_id, size_t width,
  size_t height, size_t buf_count)
    : m_renderer(renderer), m_width(width), m_height(height) {
  {
    std::shared_ptr<Renderer> renderer = m_renderer.lock();
    m_swapchain = renderer->create_swapchain(wnd_id, width, height, buf_count);
  }
}

Viewport::~Viewport() {
  if (m_swapchain != c_empty_handle) {
    std::shared_ptr<Renderer> renderer = m_renderer.lock();
    renderer->release_swapchain(m_swapchain);
  }
}

} // namespace rei