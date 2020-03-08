#include "imgui.h"

// dear ImGui
#include <imgui.h>

namespace rei {

#define DEAR_CTX static_cast<ImGuiContext*>(this->m_dear_ctx)
#define DEAR_IO static_cast<ImGuiIO*>(this->m_dear_ctx_io)

ImGUI::ImGUI() {
  IMGUI_CHECKVERSION();
  ImGuiContext* ctx = ImGui::CreateContext(); 
  ImGui::SetCurrentContext(ctx);
  ImGuiIO& io = ImGui::GetIO();
  m_dear_ctx = ctx;
  m_dear_ctx_io = &io;
  // setup imgui
  io.BackendPlatformName = "imgui_impl_dx12";
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  ImGui::StyleColorsDark();
}

void ImGUI::begin_new_frame() {
  set_to_dear_current();
  ImGui::NewFrame();
}

void ImGUI::set_display_size(float width, float height) {
  DEAR_IO->DisplaySize = { width, height };
}

void ImGUI::update_mouse_pos(float left, float top) {
  DEAR_IO->MousePos.x = left;
  DEAR_IO->MousePos.y = top;
}

void ImGUI::update_mouse_down(unsigned int index, bool down) {
  DEAR_IO->MouseDown[index] = down;
  DEAR_IO->MouseClickedPos[index] = DEAR_IO->MousePos;
}

void ImGUI::update_mouse_clicked(unsigned int index, bool clicked) {
  DEAR_IO->MouseClicked[index] = clicked;
}

ImGUI::RenderData ImGUI::prepare_render_data(bool* demoOpenState) {
  set_to_dear_current();
  if (demoOpenState) ImGui::ShowDemoWindow(demoOpenState);
  ImGui::Render();
  RenderData ret { ImGui::GetDrawData() };
  return ret;
}

void ImGUI::set_to_dear_current() {
  ImGui::SetCurrentContext(DEAR_CTX);
}

#undef DEAR_CTX
#undef DEAR_IO

#define DEAR_DRAWDATA static_cast<const ImDrawData*>(this->dearDrawDataPtr)

size_t ImGUI::RenderData::total_index_count() const {
  return DEAR_DRAWDATA->TotalIdxCount;
}

size_t ImGUI::RenderData::total_vertex_count() const {
  return DEAR_DRAWDATA->TotalVtxCount;
}

Rectf ImGUI::RenderData::viewport() const {
  float x = DEAR_DRAWDATA->DisplayPos.x;
  float y = DEAR_DRAWDATA->DisplayPos.y;
  float w = DEAR_DRAWDATA->DisplaySize.x;
  float h = DEAR_DRAWDATA->DisplaySize.y;
  return Rectf {x, y, w, h};
}

unsigned char ImGUI::RenderData::index_bytesize() {
  return sizeof(ImDrawIdx);
}

unsigned char ImGUI::RenderData::vertec_bytesize() {
  return sizeof(ImDrawVert);
}

unsigned char ImGUI::RenderData::pos_offset() {
  return IM_OFFSETOF(ImDrawVert, pos);
}

unsigned char ImGUI::RenderData::uv_offset() {
  return IM_OFFSETOF(ImDrawVert, uv);
}

unsigned char ImGUI::RenderData::color_offset() {
  return IM_OFFSETOF(ImDrawVert, col);
}

#undef DEAR_DRAWDATA

} // namespace rei