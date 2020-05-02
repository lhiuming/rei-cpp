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

void ImGUI::get_font_atlas_RGBA32(
  unsigned int* out_width, unsigned int* out_height, const unsigned char** out_pixels) {
  unsigned char* pixels;
  int width, height;
  DEAR_IO->Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  if (out_width) *out_width = width;
  if (out_height) *out_height = height;
  if (out_pixels) *out_pixels = pixels;
}

void ImGUI::set_font_atlas(BufferHandle handle) {
  DEAR_IO->Fonts->TexID = handle.get();
}

void ImGUI::begin_new_frame() {
  set_to_dear_current();
  // Dummy call to generate font atlas; actually texture creation is delayed to render pipeline.
  unsigned char* pixels;
  DEAR_IO->Fonts->GetTexDataAsRGBA32(&pixels, nullptr, nullptr);
  ImGui::NewFrame();
}

void ImGUI::set_display_size(float width, float height) {
  DEAR_IO->DisplaySize = {width, height};
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

ImGUI::RenderDataRef ImGUI::prepare_render_data(bool* demoOpenState) {
  set_to_dear_current();
  if (demoOpenState) ImGui::ShowDemoWindow(demoOpenState);
  ImGui::Render();
  RenderDataRef ret {ImGui::GetDrawData()};
  return ret;
}

void ImGUI::set_to_dear_current() {
  ImGui::SetCurrentContext(DEAR_CTX);
}

#undef DEAR_CTX
#undef DEAR_IO

#define DEAR_DRAWDATA static_cast<const ImDrawData*>(this->dearDrawDataPtr)

size_t ImGUI::RenderDataRef::total_index_count() const {
  return DEAR_DRAWDATA->TotalIdxCount;
}

size_t ImGUI::RenderDataRef::total_vertex_count() const {
  return DEAR_DRAWDATA->TotalVtxCount;
}

RenderViewport ImGUI::RenderDataRef::viewport() const {
  float x = DEAR_DRAWDATA->DisplayPos.x;
  float y = DEAR_DRAWDATA->DisplayPos.y;
  float w = DEAR_DRAWDATA->DisplaySize.x;
  float h = DEAR_DRAWDATA->DisplaySize.y;
  return RenderViewport {x, y, w, h};
}

size_t ImGUI::RenderDataRef::ui_window_count() const {
  return DEAR_DRAWDATA->CmdListsCount;
}

ImGUI::WindowDataRef ImGUI::RenderDataRef::get_ui_window_data(size_t index) const {
  REI_ASSERT(index < DEAR_DRAWDATA->CmdListsCount);
  return WindowDataRef(DEAR_DRAWDATA->CmdLists[index]);
}

unsigned char ImGUI::RenderDataRef::index_bytesize() {
  return sizeof(ImDrawIdx);
}

unsigned char ImGUI::RenderDataRef::vertec_bytesize() {
  return sizeof(ImDrawVert);
}

unsigned char ImGUI::RenderDataRef::pos_offset() {
  return IM_OFFSETOF(ImDrawVert, pos);
}

unsigned char ImGUI::RenderDataRef::uv_offset() {
  return IM_OFFSETOF(ImDrawVert, uv);
}

unsigned char ImGUI::RenderDataRef::color_offset() {
  return IM_OFFSETOF(ImDrawVert, col);
}

#undef DEAR_DRAWDATA

#define DEAR_CMDLIST static_cast<ImDrawList*>(this->dearDrawListPtr)

size_t ImGUI::WindowDataRef::total_draw_count() const {
  return DEAR_CMDLIST->CmdBuffer.size();
}

ImGUI::DrawCmdRef ImGUI::WindowDataRef::get_ui_draw_cmd(size_t index) const {
  REI_ASSERT(index < DEAR_CMDLIST->CmdBuffer.size());
  return DrawCmdRef(&(DEAR_CMDLIST->CmdBuffer[index]));
}

void ImGUI::WindowDataRef::GetGeometry(
  LowLevelGeometryData& indices, LowLevelGeometryData& vertices) const {
  indices.addr = DEAR_CMDLIST->IdxBuffer.Data;
  indices.element_count = DEAR_CMDLIST->IdxBuffer.Size;
  indices.element_bytesize = sizeof(ImDrawIdx);
  vertices.addr = DEAR_CMDLIST->VtxBuffer.Data;
  vertices.element_count = DEAR_CMDLIST->VtxBuffer.Size;
  vertices.element_bytesize = sizeof(ImDrawVert);
}

void ImGUI::WindowDataRef::GetGeometryInfo(
  unsigned int& idx_count, unsigned int& vert_count) const {
  idx_count = DEAR_CMDLIST->IdxBuffer.Size;
  vert_count = DEAR_CMDLIST->VtxBuffer.Size;
}

#undef DEAR_CMDLIST

#define DEAR_DRAWCMD static_cast<ImDrawCmd*>(this->dearDrawCmdPtr)

bool ImGUI::DrawCmdRef::handle_ui_callback(const ImGUI::WindowDataRef& win) const {
  ImDrawCallback& cb = DEAR_DRAWCMD->UserCallback;
  if (cb != NULL) {
    // User callback, registered via ImDrawList::AddCallback()
    // (ImDrawCallback_ResetRenderState is a special callback value used by the user to
    // request the renderer to reset render state.)
    if (cb == ImDrawCallback_ResetRenderState)
      REI_WARNING("Ignored callback");
    else {
      const ImDrawList* draw_list = static_cast<const ImDrawList*>(win.dearDrawListPtr);
      cb(draw_list, DEAR_DRAWCMD);
    }
    return true;
  }
  return false;
}

bool ImGUI::DrawCmdRef::use_texture(BufferHandle handle) const {
  return DEAR_DRAWCMD->TextureId == handle.get();
}

RenderArea ImGUI::DrawCmdRef::get_scissor(const RenderViewport& viewport) const {
  // check imgui.cpp and comfirm this routine...
  ImDrawCmd* pcmd = DEAR_DRAWCMD;
  ImVec4 clipRect = pcmd->ClipRect;
  int clipW = clipRect.z - clipRect.x;
  int clipH = clipRect.w - clipRect.y;
  return RenderArea {int(clipRect.x - viewport.x), int(clipRect.y - viewport.y), clipW, clipH};
}

unsigned int ImGUI::DrawCmdRef::index_count() const {
  return DEAR_DRAWCMD->ElemCount;
}

unsigned int ImGUI::DrawCmdRef::index_offset() const {
  return DEAR_DRAWCMD->IdxOffset;
}

unsigned int ImGUI::DrawCmdRef::vertex_offset() const {
  return DEAR_DRAWCMD->VtxOffset;
}

#undef DEAR_DRAWCMD

} // namespace rei