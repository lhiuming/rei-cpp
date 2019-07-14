#ifndef REI_SHADER_STRUCT_H
#define REI_SHADER_STRUCT_H

#include <vector>

namespace rei {

enum ShaderDataType {
  Float4,
  Float4x4,
};

struct ShaderTableEntry {
  size_t buffer_count;
};

struct ConstBufferLayout {
  std::vector<ShaderDataType> members;

  ConstBufferLayout(size_t member_num = 1) : members(member_num) {}

  size_t size() const { return members.size(); }

  ShaderDataType& operator[](size_t i) {
    if (i >= members.size()) { members.resize(i); }
    return members[i];
  }
  ShaderDataType operator[](size_t i) const { return const_cast<ConstBufferLayout*>(this)->operator[](i); }
};

struct DefaultBufferFormat {
  ResourceFormat format;
  ResourceDimension dimension;
};

} // namespace rei

#endif