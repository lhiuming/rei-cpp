#pragma once

#include "render_pass.h"
#include "renderer.h"
#include "renderer/graphics_handle.h"

namespace rei {

class ImGUI;

class ImGuiPass : public RenderPass {
public:
  ImGuiPass(std::weak_ptr<Renderer> renderer);

  struct Parameters {
    BufferHandle render_target;
  };
  void run(const Parameters& params);

private:
  std::weak_ptr<Renderer> m_renderer;

  bool m_opened = false;

  ShaderHandle m_imgui_shader;
  BufferHandle m_imgui_constbuffer;
  BufferHandle m_imgui_index_buffer;
  BufferHandle m_imgui_vertex_buffer;
  BufferHandle m_imgui_fonts_texture;
  ShaderArgumentHandle m_fonts_arg;
};

} // namespace rei
