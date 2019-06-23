#ifndef REI_INTPUT_H
#define REI_INTPUT_H

#include <array>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

#include "algebra.h"

namespace rei {

// Utility for variant
// TODO maybe move to somewhere like type_utils.h or enum.h
template <typename T>
struct tag {};

template <typename T, typename V>
struct alternative_index; // not defined

template <typename T, typename... Ts>
struct alternative_index<T, std::variant<Ts...> >
    : std::integral_constant<std::size_t, std::variant<tag<Ts>...>(tag<T>()).index()> {};

template <typename T, typename... Ts>
inline constexpr std::size_t alternative_index_v = alternative_index<T, Ts...>::value;

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
};


// clang-format off
typedef std::variant<
  CursorPress, 
  CursorRelease, 
  CursorMove, 
  CursorDrag
> InputVariant;
// clang-format on

// Type-safe input data
struct Input : InputVariant {
  using Index = std::size_t;
  static constexpr std::size_t variant_size = std::variant_size_v<InputVariant>;

  template <typename T>
  static inline constexpr Index get_index() {
    return alternative_index_v<T, InputVariant>;
  }

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
    Input::Index index = Input::get_index<T>();
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
