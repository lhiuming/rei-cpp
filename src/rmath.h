#ifndef REI_MATH_H
#define REI_MATH_H

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

}

#endif