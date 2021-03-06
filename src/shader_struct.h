#ifndef REI_SHADER_STRUCT_H
#define REI_SHADER_STRUCT_H

#include <vector>

namespace rei {

// Restriting types to make life easier
// ref: https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules
enum ShaderDataType {
  Float4,
  Float4x4,
};

struct ShaderTableEntry {
  size_t buffer_count;
};

struct ConstBufferLayout {
  std::vector<ShaderDataType> m_members;

  ConstBufferLayout(size_t member_num = 1) : m_members(member_num) {}
  ConstBufferLayout(std::initializer_list<ShaderDataType> members) : m_members(members) {}

  size_t size() const { return m_members.size(); }

  ShaderDataType& operator[](size_t i) {
    if (i >= m_members.size()) { m_members.resize(i + 1); }
    return m_members[i];
  }
  ShaderDataType operator[](size_t i) const {
    return const_cast<ConstBufferLayout*>(this)->operator[](i);
  }
};

} // namespace rei

#endif