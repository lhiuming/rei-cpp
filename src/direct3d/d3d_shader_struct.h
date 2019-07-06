#ifndef REI_D3D_SHADER_STRUCT_H
#define REI_D3D_SHADER_STRUCT_H

namespace rei {

namespace d3d {

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

constexpr wchar_t* c_hit_group_name = L"hit_group0";
constexpr wchar_t* c_raygen_shader_name = L"raygen_shader";
constexpr wchar_t* c_closest_hit_shader_name = L"closest_hit_shader";
constexpr wchar_t* c_miss_shader_name = L"miss_shader";

} // namespace dxr

} // namespace d3d

} // namespace rei

#endif