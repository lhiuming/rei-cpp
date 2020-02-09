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
  while (*c_str != '\0') {
    ret.push_back(wchar_t(*c_str));
    c_str++;
  }
  return ret;
}

static inline int to_cstr(const wchar_t* wchars, char* buffer) {
  int i = 0;
  while (*wchars != '\0') {
    // TODO this will overflow
    buffer[i++] = char(*wchars);
    wchars++;
  }
  buffer[i++] = '\0';
  return i;
}

template <typename... Args>
static inline std::wstring format(const wchar_t* fmt, Args... args) {
  static const std::wstring s_failed_result = std::wstring("<Failed String Formatting>");
  int buf_size = std::swprintf(nullptr, 0, fmt, args...) + 1;
  if (buf_size <= 0) return s_failed_result;
  char* buf = new char[buf_size];
  std::swprintf(buf, buf_size, fmt, args...);
  std::wstring rtn = std::wstring(buff, buf_size);
  delete[] buf;
  return rtn;
}

} // namespace rei

#endif
