#if DIRECT3D_ENABLED
// Source of d3d_renderer.h
#include "d3d_renderer.h"

#include <d3d11.h> // remove this

#include <d3d12.h>
#include <d3dx12.h>

#include "../common.h"

#include "d3d_device_resources.h"
#include "d3d_viewport_resources.h"

using std::shared_ptr;
using std::weak_ptr;

namespace rei {

using ViewportHandle = Renderer::ViewportHandle;

// Default Constructor
D3DRenderer::D3DRenderer(HINSTANCE hinstance) : hinstance(hinstance) {
  device_resources = std::make_unique<D3DDeviceResources>(hinstance);
  device_resources->create_render_states();
  device_resources->compile_shader();
}

D3DRenderer::~D3DRenderer() {
  console << "D3DRenderer is destructed." << std::endl;
}

ViewportHandle D3DRenderer::create_viewport(SystemWindowID window_id, int width, int height) {
  ASSERT(window_id.platform == SystemWindowID::Win);

  HWND hwnd = window_id.value.hwnd;

  auto d3d_vp = std::make_shared<D3D11_VIEWPORT>();
  d3d_vp->TopLeftX = 0.0f;
  d3d_vp->TopLeftY = 0.0f;
  d3d_vp->Width = width;
  d3d_vp->Height = height;
  d3d_vp->MinDepth = 0.0f;
  d3d_vp->MaxDepth = 1.0f;

  auto vp_res
    = std::make_shared<D3DViewportResources>(device_resources->device, hwnd, width, height);

  auto vp = std::make_shared<D3DViewportData>(this);
  vp->d3d_viewport = d3d_vp;
  vp->viewport_resources = vp_res;

  viewports.push_back(d3d_vp);
  viewport_resources.push_back(vp_res);
  return vp;
}

void D3DRenderer::update_viewport_vsync(ViewportHandle viewport_handle, bool enabled_vsync) {
  auto viewport = get_viewport(viewport_handle);
  ASSERT(viewport);
  viewport->enable_vsync = enabled_vsync;
}

void D3DRenderer::update_viewport_size(ViewportHandle viewport, int width, int height) {
  error("Method is not implemented");
}

void D3DRenderer::update_viewport_transform(ViewportHandle h_viewport, const Camera& camera) {
  shared_ptr<D3DViewportData> viewport = get_viewport(h_viewport);
  ASSERT(viewport);
  viewport->view_proj = camera.get_w2n();
}

void D3DRenderer::set_scene(std::shared_ptr<const Scene> scene) {
  device_resources->set_scene(scene);
}

void D3DRenderer::render(const ViewportHandle viewport_handle) {
  auto& dev_res = *device_resources;
  auto d3d11DevCon = dev_res.d3d11DevCon;
  auto data_per_frame = dev_res.data_per_frame;
  auto cbPerFrameBuffer = dev_res.cbPerFrameBuffer;
  auto g_light = dev_res.g_light;

  shared_ptr<D3DViewportData> p_viewport = get_viewport(viewport_handle);
  ASSERT(p_viewport);
  ASSERT(!p_viewport->viewport_resources.expired());
  ASSERT(!p_viewport->d3d_viewport.expired());
  D3DViewportData& viewport = *p_viewport;
  auto& vp_res = *(viewport.viewport_resources.lock());
  auto& d3d_vp = viewport.d3d_viewport.lock();
  auto depthStencilView = vp_res.depthStencilView;
  auto renderTargetView = vp_res.renderTargetView;

  float bgColor[4] = {0.3f, 0.6f, 0.7f, 0.5f};
  device_resources->d3d11DevCon->ClearRenderTargetView(renderTargetView, bgColor);

  // Also clear the depth buffer
  d3d11DevCon->ClearDepthStencilView(depthStencilView, // the view to clear
    D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, // specify the part of the depth/stencil view to clear
    1.0f, // clear value for depth; should set to the furtheast value (we use 1.0 as furthest)
    0     // clear value for stencil; we actually not using stencil currently
  );

  // Set the Viewport (bind to the Raster Stage of he pipeline)
  d3d11DevCon->RSSetViewports(1, d3d_vp.get());
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

  // present
  if (p_viewport->enable_vsync) {
    vp_res.SwapChain->Present(1, 0);
  } else {
    vp_res.SwapChain->Present(0, 0);
  }
}

// Render the default scene
void D3DRenderer::render_default_scene(D3DRenderer::D3DViewportData& viewport) {
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

  using VertexElement = D3DDeviceResources::VertexElement;

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
void D3DRenderer::render_meshes(D3DViewportData& viewport) {
  auto& dev_res = *device_resources;
  auto& d3d11DevCon = dev_res.d3d11DevCon;
  auto& mesh_buffers = dev_res.mesh_buffers;

  auto& FaceRender = dev_res.FaceRender;

  auto& view_proj_mat = viewport.view_proj;

  using VertexElement = D3DDeviceResources::VertexElement;

  // All mesh use TRIANGLELIST mode
  d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // for each meshBuffer
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
}

} // namespace rei

#endif