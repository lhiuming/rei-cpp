#pragma once

#include "renderer.h"

namespace rei {

// Wraping around an external ImGUI implementation.
class ImGUI {
public:
  class DrawCmdRef;
  class WindowDataRef;
  class RenderDataRef;

  class DrawCmdRef {
  public:
    bool handle_ui_callback(const WindowDataRef& winRef) const;
    bool use_texture(BufferHandle handle) const;
    RenderArea get_scissor(const RenderViewport& viewport) const;

    unsigned int index_count() const;
    unsigned int index_offset() const;
    unsigned int vertex_offset() const;

  private:
    void* dearDrawCmdPtr;
    DrawCmdRef(void* ptr) : dearDrawCmdPtr(ptr) {}
    friend WindowDataRef;
  };

  class WindowDataRef {
  public:
    size_t total_draw_count() const;
    DrawCmdRef get_ui_draw_cmd(size_t index) const;
    void GetGeometry(LowLevelGeometryData& indices, LowLevelGeometryData& vertices) const;
    void GetGeometryInfo(unsigned int& idx_count, unsigned int& vert_count) const;

  private:
    void* dearDrawListPtr;
    WindowDataRef(void* ptr) : dearDrawListPtr(ptr) {}
    friend RenderDataRef;
    friend DrawCmdRef;
  };

  class RenderDataRef {
  public:
    size_t total_index_count() const;
    size_t total_vertex_count() const;
    RenderViewport viewport() const;

    size_t ui_window_count() const;
    WindowDataRef get_ui_window_data(size_t index) const;

    static unsigned char index_bytesize();
    static unsigned char vertec_bytesize();
    static unsigned char pos_offset();
    static unsigned char uv_offset();
    static unsigned char color_offset();

  private:
    void* dearDrawDataPtr;
    RenderDataRef(void* ptr) : dearDrawDataPtr(ptr) {};
    friend ImGUI;
  };

  ImGUI();

  void get_font_atlas_RGBA32(
    unsigned int* width, unsigned int* height, const unsigned char** atlas);
  void set_font_atlas(BufferHandle handle);

  void begin_new_frame();
  void set_display_size(float width, float height);

  void update_mouse_pos(float top, float left);
  void update_mouse_down(unsigned int index, bool down = true);
  void update_mouse_clicked(unsigned int index, bool clicked = true);

  void test();
  void show_imgui_demo();

  bool show_collapsing_header(const char* label);
  void show_checkbox(const char* label, bool* value);
  void show_seperator();
  void show_tex(const char* text);

  RenderDataRef prepare_render_data();

private:
  void* m_dear_ctx;
  void* m_dear_ctx_io;

  void set_to_dear_current();
};

} // namespace rei