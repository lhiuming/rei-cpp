#ifndef REI_INTPUT_H
#define REI_INTPUT_H

#include <array>
#include <vector>

#include "algebra.h"
#include "variant_utils.h"
#include "debug.h"

namespace rei {

struct _CursorPointData {
  Vec3 coord;
  _CursorPointData(float x, float y) : coord(Vec3(x, y, 0)) {}
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
struct CursorDown {};
struct CursorUp {};
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
  , CursorDown
  , CursorUp
  , Zoom
> Input;
// clang-format on

class InputBus {
public:
  using InputBucket = std::vector<Input>;
  using Inputs = std::array<InputBucket, Input::variant_size>;

  template <typename T>
  struct InputBucketItr {
    using Self = InputBucketItr;

    using iterator_category = std::bidirectional_iterator_tag;
    using reference = const T&;

    const InputBucket& bkt;
    size_t idx;

    InputBucketItr(const InputBucket& buket, size_t i) : bkt(buket), idx(i) {}

    reference operator*() const {
      REI_ASSERT(idx < bkt.size());
      return bkt[idx].get<T>();
    }

    Self& operator++() {
      ++idx;
      return *this;
    }

    Self operator++(int) { return Self(bkt, idx++); }

    Self& operator--() {
      --idx;
      return *this;
    }

    Self operator--(int) { return Self(bkt, idx--); }

    bool operator==(const Self& rhs) const { return idx == rhs.idx; }
    bool operator!=(const Self& rhs) const { return !(*this == rhs); }

    bool operator<(const Self& rhs) const { return idx < rhs.idx; }
    bool operator>(const Self& rhs) const { return *this < rhs; }
    bool operator<=(const Self& rhs) const { return !(rhs < *this); }
    bool operator>=(const Self& rhs) const { return !(*this < rhs); }
  };

  template <typename T>
  struct InputBucketProxy {
    const InputBucket& bkt;
    InputBucketProxy(const InputBucket& bucket) : bkt(bucket) {}

    InputBucketItr<T> begin() { return InputBucketItr<T>(bkt, 0); }
    InputBucketItr<T> end() { return InputBucketItr<T>(bkt, bkt.size()); }

    size_t size() const { return bkt.size(); }
  };

  InputBus() : m_inputs() {}

  void push(const Input& value) { m_inputs[value.index()].push_back(value); }

  void set_cursor_range(Vec3 left_top, Vec3 right_bottom) {
    m_cursor_left_top = left_top;
    m_cursor_right_bottom = right_bottom;
  }

  Vec3 cursor_left_top() const { return m_cursor_left_top; }
  Vec3 cursor_top_left() const { return m_cursor_left_top; }
  Vec3 cursor_right_bottom() const { return m_cursor_right_bottom; }
  Vec3 cursor_bottom_right() const { return m_cursor_right_bottom; }

  template <typename T>
  bool has_any() {
    return get<T>().size();
  }

  template <typename T>
  InputBucketProxy<T> get() const {
    constexpr size_t index = Input::get_index<T>();
    return InputBucketProxy<T>(m_inputs[index]);
  }

  template <typename T>
  bool empty() const {
    constexpr size_t index = Input::get_index<T>();
    return m_inputs[index].empty();
  }

  void reset() {
    for (std::size_t i = 0; i < Input::variant_size; i++) {
      m_inputs[i].clear();
    }
  }

private:
  Inputs m_inputs;
  Vec3 m_cursor_left_top;
  Vec3 m_cursor_right_bottom;
};

} // namespace rei

#endif
