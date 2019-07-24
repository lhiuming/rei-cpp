#ifndef REI_CONTAINER_UTILS_H
#define REI_CONTAINER_UTILS_H

#include <array>
#include <initializer_list>
#include <unordered_map>

#include "debug.h"

namespace rei {

// Using an array like a vector, but with fixed size
template <typename T, size_t N>
class FixedVec : private std::array<T, N> {
  using Self = FixedVec;
  using Base = std::array<T, N>;
  using Containter = std::array<T, N>;
  using C = Containter;

public:
  using C::const_pointer;
  using C::const_reference;
  using C::difference_type;
  using C::pointer;
  using C::reference;
  using C::size_type;
  using C::value_type;
  // TODO iterator

  class const_iterator { // iterator for nonmutable array
    using Self = const_iterator;

  public:
    // TODO support random access
    // using iterator_category = random_access_iterator_tag;
    using iterator_category = std::bidirectional_iterator_tag;
    using pointer = const T*;
    using reference = const T&;

    const_iterator(pointer p, size_type i) : ptr(p), idx(i) {}

    reference operator*() const { return *operator->(); }
    pointer operator->() const { return ptr + idx; }

    Self& operator++() {
      ++idx;
      return *this;
    }

    Self operator++(int) { return Self(ptr, idx++); }

    Self& operator--() {
      --idx;
      return *this;
    }

    Self operator--(int) { return Self(ptr, idx--); }

    bool operator==(const Self& rhs) const {
      validate(rhs);
      return idx == rhs.idx;
    }
    bool operator!=(const Self& rhs) const { return !(*this == rhs); }

    bool operator<(const Self& rhs) const {
      validate(rhs);
      return idx < rhs.idx;
    }
    bool operator>(const Self& rhs) const { return *this < rhs; }
    bool operator<=(const Self& rhs) const { return !(rhs < *this); }
    bool operator>=(const Self& rhs) const { return !(*this < rhs); }

  private:
    pointer ptr = nullptr;
    size_t idx = 0;

    void validate(const Self& rhs) const { REI_ASSERT(ptr == rhs.ptr); }
  };

  FixedVec() {}

  FixedVec(size_t size) : m_size(size) {}

  constexpr FixedVec(const std::initializer_list<T>& l) : m_size(l.size()) {
    REI_ASSERT(l.size() <= N);
    // static_assert(M <= N, "initializer size is greater than v_array max size");
    std::copy(l.begin(), l.end(), ((Base*)this)->begin());
  }

  // TODO do we need modifiable iterator?
  const_iterator cbegin() const { return const_iterator(this->data(), 0); }
  const_iterator begin() const { return const_iterator(this->data(), 0); }
  const_iterator cend() const { return const_iterator(this->data(), m_size); }
  const_iterator end() const { return const_iterator(this->data(), m_size); }

  constexpr reference operator[](size_type i) {
    REI_ASSERT(i < m_size);
    return ((Base*)this)->operator[](i);
  }
  constexpr const_reference operator[](size_type i) const {
    REI_ASSERT(i < m_size);
    return const_cast<Self*>(this)->operator[](i);
  }

  void push_back(value_type&& v) {
    REI_ASSERT(m_size < N);
    *(this->data() + m_size) = std::move(v);
    m_size++;
  }

  void push_back(const value_type& v) {
    REI_ASSERT(m_size < N);
    *(this->data() + m_size) = v;
    m_size++;
  }

  template <typename... Args>
  reference emplace_back(Args&&... args) {
    REI_ASSERT(m_size < N);
    reference r = *(this->data() + m_size);
    r = T(std::forward<Args>(args)...);
    m_size++;
    return r;
  }

  reference back() { return *this[m_size]; }
  const_reference back() const { return *this[m_size]; }

  void clear() {
    for (auto& ele : *this) {
      ele.~T();
    }
    m_size = 0;
  }

  size_type size() const { return m_size; }
  constexpr size_type max_size() const { return N; }

  using Base::data;

private:
  size_type m_size = 0;
};

// TODO fast hashmap
template <typename TKey, typename TVal>
class Hashmap : public std::unordered_map<TKey, TVal> {
public:
  TVal* try_get(TKey key) {
    auto found = this->find(key);
    if (found != this->end()) { return &(found->second); }
    return nullptr;
  }
};

} // namespace rei

#endif