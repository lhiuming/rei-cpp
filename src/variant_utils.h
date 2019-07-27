#ifndef REI_VARIANT_UTILS_H
#define REI_VARIANT_UTILS_H

#include <type_traits>
#include <variant>
#include <optional>
#include <ostream>

namespace rei {

namespace variant_utils {

template <typename T>
struct tag {};

template <typename T, typename V>
struct alternative_index; // not defined

template <typename T, typename... Ts>
struct alternative_index<T, std::variant<Ts...> >
    : std::integral_constant<std::size_t, std::variant<tag<Ts>...>(tag<T>()).index()> {};

template <typename T, typename... Ts>
inline constexpr std::size_t alternative_index_v = alternative_index<T, Ts...>::value;

// NOTE: stole from https://en.cppreference.com/w/cpp/utility/variant/visit
template <typename... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

template <typename... Ts>
overloaded(Ts...)->overloaded<Ts...>;

} // namespace variant_utils

// Printing monostate
inline std::wostream& operator<<(std::wostream& os, const std::monostate& mono) {
  return os << "<empty>";
}

// std::variant with additional useful methods
template<typename... Args>
struct Var : std::variant<Args...> {
  using Base = std::variant<Args...>;
  using Base::Base;

  static constexpr std::size_t variant_size = std::variant_size_v<Base>;

  template <typename T>
  static inline constexpr size_t get_index() {
    return variant_utils::alternative_index_v<T, Base>;
  }

  template<typename T>
  bool holds() const {
    return this->index() == get_index<T>();
  }

  template<typename T0, typename... Ts>
  bool holds_any() const {
    return holds<T0>() || holds_any<Ts...>();
  }

  template<typename T0>
  bool holds_any() const {
    return holds<T0>();
  }

  // hack to std::get
  Var& self_reference() { return *this; }
  const Var& self_reference() const { return *this; }

  template<typename T>
  std::remove_reference_t<T>& get() {
    return std::get<T>(self_reference());
  }

  template<typename... Lmbds>
  auto match(Lmbds... lambds) {
    return std::visit(variant_utils::overloaded {lambds...}, self_reference());
  }

  friend std::wostream& operator<<(std::wostream& os, const Var<Args...>& v) {
    std::visit([&](const auto& arg) { os << arg; }, v);
    return os;
  }
};

} // namespace rei

#endif
