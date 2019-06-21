#if DIRECT3D_ENABLED
// Source of d3d_renderer.h
#include "d3d_renderer.h"

#include <d3d11.h> // remove this

#include <d3d12.h>
#include <d3dx12.h>

#include "../debug.h"

#include "d3d_common_resources.h"
#include "d3d_device_resources.h"
#include "d3d_viewport_resources.h"

using std::shared_ptr;
using std::weak_ptr;
using std::make_shared;

namespace rei {

  namespace d3d {

using ViewportHandle = Renderer::ViewportHandle;
using ShaderID = Renderer::ShaderHandle;

// Default Constructor
Renderer::Renderer(HINSTANCE hinstance) : hinstance(hinstance) {
  device_resources = std::make_unique<DeviceResources>(hinstance);
}

Renderer::~Renderer() {
  log("D3DRenderer is destructed.");
}

ViewportHandle Renderer::create_viewport(SystemWindowID window_id, int width, int height) {
  ASSERT(window_id.platform == SystemWindowID::Win);

  HWND hwnd = window_id.value.hwnd;

  /*
   * NOTE: DirectX viewport starts from top-left to bottom-right.
   */

  D3D12_VIEWPORT d3d_vp {};
  d3d_vp.TopLeftX = 0.0f;
  d3d_vp.TopLeftY = 0.0f;
  d3d_vp.Width = width;
  d3d_vp.Height = height;
  d3d_vp.MinDepth = 0.0f;
  d3d_vp.MaxDepth = 1.0f;

  D3D12_RECT scissor {};
  scissor.left = 0;
  scissor.top = 0;
  scissor.right = width;
  scissor.bottom = height;

  auto vp_res
    = std::make_shared<ViewportResources>(device_resources->device, device_resources->dxgi_factory, device_resources->command_queue, hwnd, width, height);

  auto vp = std::make_shared<ViewportData>(this);
  vp->clear_color = {0.f, 0.f, 0.f, 0.5f}; 
  vp->d3d_viewport = d3d_vp;
  vp->viewport_resources = vp_res;
  vp->scissor = scissor;

  viewport_resources_lib.push_back(vp_res);
  return vp;
}

void Renderer::set_viewport_clear_value(ViewportHandle viewport_handle, Color c) {
  auto viewport = get_viewport(viewport_handle);
  viewport->clear_color = {c.r, c.g, c.b, c.a};
}

void Renderer::update_viewport_vsync(ViewportHandle viewport_handle, bool enabled_vsync) {
  auto viewport = get_viewport(viewport_handle);
  ASSERT(viewport);
  viewport->enable_vsync = enabled_vsync;
}

void Renderer::update_viewport_size(ViewportHandle viewport, int width, int height) {
  error("Method is not implemented");
}

void Renderer::update_viewport_transform(Renderer::ViewportHandle h_viewport, const Camera& camera) {
  shared_ptr<ViewportData> viewport = get_viewport(h_viewport);
  ASSERT(viewport);
  viewport->view_proj = camera.get_w2n();
}

void Renderer::set_scene(std::shared_ptr<const Scene> scene) {
  for (const auto& modelIns : scene->get_models()) {
    NOT_IMPLEMENT();
    // add_mesh_buffer(modelIns);
  }
}

Renderer::ShaderHandle Renderer::create_shader(std::wstring shader_path) {
  // TODO crate root signature

  auto shader = make_shared<ShaderData>(this);
  this->device_resources->compile_shader(shader_path, shader->compiled_data);

  // fetch PSO ?

  return ShaderHandle(shader);
}

Renderer::GeometryHandle Renderer::create_geometry(const Geometry& geometry) {
  // TODO currently only support mesh type
  auto& mesh = dynamic_cast<const Mesh&>(geometry);
  auto mesh_data = make_shared<MeshData>(this);
  device_resources->create_mesh_buffer(mesh, *mesh_data);
  return GeometryHandle(mesh_data);
}

void Renderer::render(const ViewportHandle viewport_handle) {
  shared_ptr<ViewportData> p_viewport = get_viewport(viewport_handle);
  ASSERT(p_viewport);
  render(*p_viewport);
}

void Renderer::render(ViewportData& viewport) {
  ASSERT(device_resources.get());
  ASSERT(!viewport.viewport_resources.expired());

  auto& dev_res = *device_resources;
  auto& vp_res = *(viewport.viewport_resources.lock());

  // device shorcuts
  const ComPtr<ID3D12CommandAllocator> cmd_alloc = dev_res.command_alloc;
  const ComPtr<ID3D12GraphicsCommandList> cmd_list = dev_res.command_list;
  const ComPtr<ID3D12CommandQueue> cmd_queue = dev_res.command_queue;
  auto d3d11DevCon = dev_res.d3d11DevCon;
  auto data_per_frame = dev_res.data_per_frame;
  auto cbPerFrameBuffer = dev_res.cbPerFrameBuffer;
  auto g_light = dev_res.g_light;

  // viewport shortcuts
  const D3D12_VIEWPORT& d3d_vp = viewport.d3d_viewport;
  const D3D12_RECT& scissor = viewport.scissor;
  const RenderTargetSpec& target_spec = vp_res.target_spec;
  // deprecated
  auto depthStencilView = vp_res.depthStencilView;
  auto renderTargetView = vp_res.renderTargetView;

  // Prepare
  ASSERT(SUCCEEDED(cmd_alloc->Reset())); // assume previous GPU tasks are finished
  ASSERT(SUCCEEDED(cmd_list->Reset(
    cmd_alloc.Get(), NULL))); // reuse the command list object; fine with null init pso

  // Prepare render target
  D3D12_RESOURCE_BARRIER pre_rt
    = CD3DX12_RESOURCE_BARRIER::Transition(vp_res.get_current_rt_buffer().Get(),
      D3D12_RESOURCE_STATE_PRESENT, // previous either COMMON of PRESENT
      D3D12_RESOURCE_STATE_RENDER_TARGET);
  cmd_list->ResourceBarrier(1, &pre_rt);

  // Set Viewport and scissor
  cmd_list->RSSetViewports(1, &d3d_vp);
  cmd_list->RSSetScissorRects(1, &scissor);

  // Specify render target
  cmd_list->OMSetRenderTargets(1, &vp_res.get_current_rtv(), true, &vp_res.get_dsv());

  // Clear
  const FLOAT* clear_color = viewport.clear_color.data();
  D3D12_RECT* entire_view = nullptr;
  cmd_list->ClearRenderTargetView(vp_res.get_current_rtv(), clear_color, 0, entire_view);
  D3D12_CLEAR_FLAGS ds_clear_flags = D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL;
  FLOAT clear_depth = target_spec.ds_clear.Depth;
  FLOAT clear_stencil = target_spec.ds_clear.Stencil;
  cmd_list->ClearDepthStencilView(
    vp_res.get_dsv(), ds_clear_flags, clear_depth, clear_stencil, 0, entire_view);

  goto finalize_render;

  device_resources->d3d11DevCon->ClearRenderTargetView(renderTargetView, clear_color);

  // Also clear the depth buffer
  d3d11DevCon->ClearDepthStencilView(depthStencilView, // the view to clear
    D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, // specify the part of the depth/stencil view to clear
    1.0f, // clear value for depth; should set to the furtheast value (we use 1.0 as furthest)
    0     // clear value for stencil; we actually not using stencil currently
  );

  // Set the Viewport (bind to the Raster Stage of he pipeline)
  // d3d11DevCon->RSSetViewports(1, d3d_vp.get());
  // Bind Render Target View and StencilDepth View to OM stage
  d3d11DevCon->OMSetRenderTargets(1, // number of render targets to bind
    &renderTargetView, depthStencilView);

  // Update and feed the global light-source data for the scene
  // FIXME : update from this->scene
  data_per_frame.light = g_light;
  d3d11DevCon->UpdateSubresource(
    cbPerFrameBuffer, 0, NULL, &data_per_frame, 0, 0);        // update into buffer object
  d3d11DevCon->PSSetConstantBuffers(0, 1, &cbPerFrameBuffer); // send to the Pixel Stage

  // Render the default scene, for debug
  this->render_default_scene(viewport);

  // render all buffered meshes
  if (scene != nullptr) { render_meshes(viewport); }

finalize_render:

  // Finish writing render target
  D3D12_RESOURCE_BARRIER post_rt
    = CD3DX12_RESOURCE_BARRIER::Transition(vp_res.get_current_rt_buffer().Get(),
      D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
  cmd_list->ResourceBarrier(1, &post_rt);

  // Finish adding commands
  ASSERT(SUCCEEDED(cmd_list->Close()));

  // Submit the only command list
  ID3D12CommandList* temp_cmd_lists[1] = {cmd_list.Get()};
  dev_res.command_queue->ExecuteCommandLists(1, temp_cmd_lists);
  //cmd_list.Reset(); // fine to reset it no

  // Present and flip
  if (viewport.enable_vsync) {
    vp_res.swapchain->Present(1, 0);
  } else {
    vp_res.swapchain->Present(0, 0);
  }
  vp_res.flip_backbuffer();

  // Flush and wait
  dev_res.flush_command_queue_for_frame();
}

// Render the default scene
void Renderer::render_default_scene(ViewportData& viewport) {
  auto& dev_res = *device_resources;
  auto& d3d11DevCon = dev_res.d3d11DevCon;

  auto& cubeVertBuffer = dev_res.cubeVertBuffer;
  auto& cubeIndexBuffer = dev_res.cubeIndexBuffer;
  auto& cube_rot = dev_res.cube_rot;
  auto& cube_cb_data = dev_res.cube_cb_data;
  auto& cubeConstBuffer = dev_res.cubeConstBuffer;
  auto& FaceRender = dev_res.FaceRender;

  auto& vp_res = *(viewport.viewport_resources.lock());
  auto& renderTargetView = vp_res.renderTargetView;
  auto& depthStencilView = vp_res.depthStencilView;
  auto& view_proj_mat = viewport.view_proj;

  using VertexElement = VertexElement;

  // Binding to the IA //

  // Set Primitive Topology (tell InputAssemble )
  d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // Bind the cube-related buffers to Input Assembler
  UINT stride = sizeof(VertexElement);
  UINT offset = 0;
  d3d11DevCon->IASetVertexBuffers(0, // the input slot we use as start
    1,                               // number of buffer to bind; we bind one buffer
    &cubeVertBuffer,                 // pointer to the buffer object
    &stride,                         // pStrides; data size for each vertex
    &offset                          // starting offset in the data
  );
  d3d11DevCon->IASetIndexBuffer( // NOTE: IndexBuffer !!
    cubeIndexBuffer,      // pointer o a buffer data object; must have  D3D11_BIND_INDEX_BUFFER flag
    DXGI_FORMAT_R32_UINT, // data format
    0                     // UINT; starting offset in the data
  );

  // Feed transform to per-object constant buffer
  cube_rot -= 0.002;
  Mat4 rotateLH = Mat4(cos(cube_rot), 0.0, -sin(cube_rot), 0.0, 0.0, 1.0, 0.0, 0.0, sin(cube_rot),
    0.0, cos(cube_rot), 0.0, 0.0, 0.0, 0.0, 1.0);
  cube_cb_data.update(rotateLH * view_proj_mat, rotateLH);
  d3d11DevCon->UpdateSubresource(cubeConstBuffer, 0, NULL, &cube_cb_data, 0, 0);
  d3d11DevCon->VSSetConstantBuffers(0, 1, &cubeConstBuffer);

  // Set rendering state
  d3d11DevCon->RSSetState(FaceRender);

  // Draw
  d3d11DevCon->DrawIndexed(36, 0, 0);
}

// Render all buffered meshes
void Renderer::render_meshes(ViewportData& viewport) {
  auto& dev_res = *device_resources;
  auto& d3d11DevCon = dev_res.d3d11DevCon;

  auto& FaceRender = dev_res.FaceRender;

  auto& view_proj_mat = viewport.view_proj;

  // All mesh use TRIANGLELIST mode
  d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // for each meshBuffer
  NOT_IMPLEMENTED
  /*
  for (auto& mb : mesh_buffers) {
    // Bind the buffers

    // 1. bind vertex buffer
    UINT stride = sizeof(VertexElement);
    UINT offset = 0;
    d3d11DevCon->IASetVertexBuffers(0, // the input slot we use as start
      1,                               // number of buffer to bind; we bind one buffer
      &(mb.meshVertBuffer),            // pointer to the buffer object
      &stride,                         // pStrides; data size for each vertex
      &offset                          // starting offset in the data
    );

    // 2. bind indices buffer
    d3d11DevCon->IASetIndexBuffer(mb.meshIndexBuffer, // pointer to a buffer data object
      DXGI_FORMAT_R32_UINT,                           // data format
      0                                               // unsigned int; starting offset in the data
    );

    // 3. update and bind the per-mesh constant Buffe
    mb.mesh_cb_data.update(view_proj_mat);
    mb.mesh_cb_data.World = DirectX::XMMatrixIdentity();
    d3d11DevCon->UpdateSubresource(mb.meshConstBuffer, 0, NULL, &(mb.mesh_cb_data), 0, 0);
    d3d11DevCon->VSSetConstantBuffers(0, 1, &mb.meshConstBuffer);

    // set render state
    d3d11DevCon->RSSetState(FaceRender);

    // 5. DrawIndexed.
    d3d11DevCon->DrawIndexed(mb.indices_num(), 0, 0);
  }
  */
}

void Renderer::rasterize_triangle(const Mesh::Triangle& tri, const Mat4& trans) {}

inline std::shared_ptr<ViewportData> Renderer::get_viewport(ViewportHandle h_viewport) {
  if (h_viewport && (h_viewport->owner == this))
    return std::static_pointer_cast<ViewportData>(h_viewport);
  return nullptr;
}

} // namespace d3d

} // namespace rei

#endif