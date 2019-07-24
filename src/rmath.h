#ifndef REI_MATH_H
#define REI_MATH_H

#include <algorithm>
#include <cmath>

// TODO check boost for existing funcionalities

namespace rei {

// via https://www.wolframalpha.com/input/?i=pi
template <typename D>
constexpr double pi_tpl = D(3.141592653589793238462643383279502884197169399375105820974);
constexpr double pi_f = pi_tpl<float>;
constexpr double pi_d = pi_tpl<double>;
constexpr double pi = pi_d;

// in radians
constexpr double degree = pi_d / 180;
constexpr double degree_inv = 180 / pi_d;

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

} // namespace rei

#endif