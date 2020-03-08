#pragma once

#include "math/rect.h"

namespace rei {

// Wraping around an external ImGUI implementation.
class ImGUI {
public:
  class RenderData {
  public:
    size_t total_index_count() const;
    size_t total_vertex_count() const;
    Rectf viewport() const;

    static unsigned char index_bytesize();
    static unsigned char vertec_bytesize();
    static unsigned char pos_offset();
    static unsigned char uv_offset();
    static unsigned char color_offset();
  
  private:
    void* dearDrawDataPtr;
    RenderData(void* ptr) : dearDrawDataPtr(ptr) {};
    friend ImGUI;
  };

  ImGUI();

  void begin_new_frame();
  void set_display_size(float width, float height);

  void update_mouse_pos(float top, float left);
  void update_mouse_down(unsigned int index, bool down = true);
  void update_mouse_clicked(unsigned int index, bool clicked = true);

  RenderData prepare_render_data(bool* demoOpenState = nullptr);

private:
  void* m_dear_ctx;
  void* m_dear_ctx_io;

  void set_to_dear_current();
};

} // namespace rei