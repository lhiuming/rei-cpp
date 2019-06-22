#ifndef REI_STRING_UTILS_H
#define REI_STRING_UTILS_H

#include <ostream>
#include <string>

namespace rei {

static inline std::wostream& operator<<(std::wostream& wos, std::string str) {
  for (auto c : str)
    wos.put(wchar_t(c));
  return wos;
}

static inline std::wstring make_wstring(const char* c_str) {
  std::wstring ret;
  while (&c_str != '\0') {
    ret.push_back(wchar_t(*c_str));
  }
  return ret;
}

} // namespace rei

#endif
