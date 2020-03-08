#pragma once

namespace rei {

template<typename NumericOffset, typename NumericSize>
struct Rect {
  NumericOffset x;
  NumericOffset y;
  NumericSize width;
  NumericSize height;
};

using Rectf = Rect<float, float>;

}