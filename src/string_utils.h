#ifndef REI_STRING_UTILS_H
#define REI_STRING_UTILS_H

#include <string>
#include <ostream>

namespace rei {

static inline std::wostream& operator<<(std::wostream& wos, std::string str) {
  for (auto c : str) wos.put(wchar_t(c));
  return wos;
}

} // namespace rei

#endif
