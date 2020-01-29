#ifndef REI_D3D_RENDERER_RESOURCE
#define REI_D3D_RENDERER_RESOURCE

#include <d3d12.h>
#include <d3dx12.h>
#include <windows.h>

#include "../debug.h"
#include "../string_utils.h"

#include "d3d_device_resources.h"
#include "d3d_shader_struct.h"

namespace rei::d3d {

struct IndexBuffer {
  CommittedResource res;
  UINT index_count;
  DXGI_FORMAT format;
};

struct VertexBuffer {
  CommittedResource res;
  UINT vertex_count;
  UINT stride;
};

struct TextureBuffer {
  ComPtr<ID3D12Resource> buffer;
  ResourceFormat format;
  ResourceDimension dimension;
#if DEBUG
  std::wstring name;
#endif
};

struct ConstBuffer {
  std::unique_ptr<UploadBuffer> buffer;
  ConstBufferLayout layout;
};

struct BlasBuffer {
  CommittedResource res;
};

struct TlasBuffer {
  ComPtr<ID3D12Resource> buffer;
};

struct RaytracingShaderData; // see below
struct ShaderTableBuffer {
  std::shared_ptr<RaytracingShaderData> shader;
  std::shared_ptr<ShaderTable> raygen;
  std::shared_ptr<ShaderTable> hitgroup;
  std::shared_ptr<ShaderTable> miss;
};

// Holds a variant of buffer/texture resources
struct BufferData : BaseBufferData {
  using BaseBufferData::BaseBufferData;
  using ResourceVariant = Var<std::monostate, IndexBuffer, VertexBuffer, TextureBuffer, ConstBuffer,
    BlasBuffer, TlasBuffer, ShaderTableBuffer>;

  ResourceVariant res;
  D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;

  ID3D12Resource* get_res();
};

} // namespace rei::d3d

#endif
