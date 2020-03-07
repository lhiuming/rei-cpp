#pragma once

#include <cmath>
//#include <algorithm>

#include "math/num_types.h"

namespace rei {

// fast integer power
template <typename D>
inline constexpr D fase_pow_tpl(D d, int p, D identity) {
  D binary_power = d;
  D ret = identity;
  while (p > 0) {
    if ((p & 0x1)) ret = ret * binary_power;
    binary_power = binary_power * binary_power;
    p = p >> 1;
  }
  return ret;
}

inline constexpr int pow_i(int i, int p) {
  return fase_pow_tpl<int>(i, p, 1);
}


}
