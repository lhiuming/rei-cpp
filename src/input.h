#ifndef REI_INTPUT_H
#define REI_INTPUT_H

#include <array>
#include <vector>

#include "algebra.h"
#include "variant_utils.h"

namespace rei {

struct _CursorPointData {
  Vec3 coord;
  _CursorPointData(float left, float bottom) : coord(Vec3(left, 0, bottom)) {}
};

struct _CursorLineData {
  Vec3 start;
  Vec3 stop;
  _CursorLineData(float x0, float y0, float x1, float y1) : start(x0, y0, 0), stop(x1, y1, 0) {}
};

// Typed input data
enum class CursorAlterType { None, Left, Right, Middle };
struct CursorPress : _CursorPointData {
  using _CursorPointData::_CursorPointData;
};
struct CursorRelease : _CursorPointData {
  using _CursorPointData::_CursorPointData;
};
struct CursorMove : public _CursorLineData {
  using _CursorLineData::_CursorLineData;
};
struct CursorDrag : _CursorLineData {
  using _CursorLineData::_CursorLineData;
  CursorAlterType alter = CursorAlterType::None;
  CursorDrag(float x0, float y0, float x1, float y1, CursorAlterType alter)
      : _CursorLineData(x0, y0, x1, y1), alter(alter) {}
};
struct Zoom {
  double delta; // positive: zoom in; negative: zoom out;
  bool altered;
};

// clang-format off
typedef Var<
  CursorPress
  , CursorRelease
  , CursorMove
  , CursorDrag
  , Zoom
> InputVariant;
// clang-format on

// Type-safe input data
struct Input : InputVariant {
  template <typename T>
  inline constexpr const T* get() const {
    return std::get_if<T>(this);
  }

  using InputVariant::InputVariant;
};

class InputBus {
public:
  using InputBucket = std::vector<Input>;
  using Inputs = std::array<InputBucket, Input::variant_size>;

  InputBus() : inputs() {}

  void push(const Input& value) { inputs[value.index()].push_back(value); }

  template <typename T>
  bool has_any() {
    return get<T>().size();
  }

  template <typename T>
  const InputBucket& get() const {
    size_t index = Input::get_index<T>();
    return inputs[index];
  }

  void reset() {
    for (std::size_t i = 0; i < Input::variant_size; i++) {
      inputs[i].clear();
    }
  }

private:
  Inputs inputs;
};

} // namespace rei

#endif
