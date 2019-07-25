#ifndef REI_VARIANT_UTILS_H
#define REI_VARIANT_UTILS_H

//#include <optional>
#include <type_traits>
#include <variant>
#include <ostream>

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

inline std::wostream& operator<<(std::wostream& os, const std::monostate& mono) {
  return os << "<empty>";
}

// Typename alias
template<typename... Args>
struct Var : std::variant<Args...> {
  using Base = std::variant<Args...>;
  using Base::Base;

friend std::wostream& operator<<(std::wostream& os, const Var<Args...>& v) {
  std::visit([&](const auto& arg) { os << arg; }, v);
  return os;
}
};

} // namespace rei

#endif
