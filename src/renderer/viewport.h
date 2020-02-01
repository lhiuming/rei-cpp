#pragma once

#include <memory>
#include <unordered_map>

#include "type_utils.h"
#include "renderer.h"

/*
 * Viewport.
 * Basically a swapchain that can be rendered into and presented.
 */

namespace rei {

class Viewport : private NoCopy {
public:
  Viewport(std::weak_ptr<Renderer> renderer, SystemWindowID wnd_id, size_t width, size_t height, size_t buf_count = 3);
  ~Viewport();

private:
  const std::weak_ptr<Renderer> m_renderer;

  size_t m_width;
  size_t m_height;
  SwapchainHandle m_swapchain;
};

} // namespace rei
