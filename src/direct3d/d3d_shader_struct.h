#ifndef REI_D3D_SHADER_STRUCT_H
#define REI_D3D_SHADER_STRUCT_H

#include "../shader_struct.h";

#include <d3d12.h>;

namespace rei {

namespace d3d {

// ref: https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-scalar
inline static UINT get_width(ShaderDataType dtype) {
  switch (dtype) {
    case rei::Float4:
      return sizeof(UINT32) * 4;
    case rei::Float4x4:
      return sizeof(UINT32) * 16;
    default:
      REI_ERROR("Unhandled data type");
      return 0;
  }
}

inline static UINT get_width(const ConstBufferLayout& layout) {
  UINT sum = 0;
  for (ShaderDataType ele : layout.m_members) {
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

} // namespace d3d

} // namespace rei

#endif