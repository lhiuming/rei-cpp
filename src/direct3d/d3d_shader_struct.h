#ifndef REI_D3D_SHADER_STRUCT_H
#define REI_D3D_SHADER_STRUCT_H

#include "../shader_struct.h";

#include <d3d12.h>;

namespace rei {

namespace d3d {

inline static UINT get_width(ShaderDataType dtype) {
  switch (dtype) {
    case rei::Float4:
      return sizeof(float) * 4;
    case rei::Float4x4:
      return sizeof(float) * 16;
    default:
      REI_ERROR("Unhandled data type");
      return 0;
  }
}

inline static UINT get_width(const ConstBufferLayout& layout) {
  UINT sum = 0;
  for (ShaderDataType ele : layout.members) {
    sum += get_width(ele);
  }
  return sum;
}

inline static UINT get_offset(const ConstBufferLayout& layout, size_t index) {
  REI_ASSERT(index < layout.size());
  UINT offset = 0;
  for (size_t i = 0; i < index; i++) {
    offset += get_width(layout[i]);
  }
  return offset;
}

namespace dxr {
/*
 * Hard code the root siagnature for dxr kick up
 */
struct GlobalRSLayout {
  enum Slots {
    OuputTextureUAV_SingleTable = 0,
    AccelStructSRV,
    PerFrameCBV,
    Count
  };
};

struct RaygenRSLayout {};

struct HitgroupRSLayout {
  enum Slots {
    MeshBuffer_Table = 0,
    PerObjectCBV, 
    Count
  };
};

struct PerObjectConstantBuffer {
};

struct HitgroupRootArguments {
  D3D12_GPU_DESCRIPTOR_HANDLE mesh_buffer_table_start;
};

// FIXME remove this hardcode
constexpr size_t c_hitgroup_size = 32;

struct PerFrameConstantBuffer {
  DirectX::XMMATRIX proj_to_world;
  DirectX::XMFLOAT4 camera_pos;
  DirectX::XMFLOAT4 ambient_color;
  DirectX::XMFLOAT4 light_pos;
  DirectX::XMFLOAT4 light_color;
};

/*
constexpr wchar_t* c_hit_group_name = L"hit_group0";
constexpr wchar_t* c_raygen_shader_name = L"raygen_shader";
constexpr wchar_t* c_closest_hit_shader_name = L"closest_hit_shader";
constexpr wchar_t* c_miss_shader_name = L"miss_shader";
*/

} // namespace dxr

} // namespace d3d

} // namespace rei

#endif