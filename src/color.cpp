// source of color.h
#include "color.h"

namespace rei {

// Print
std::wostream& operator<<(std::wostream& os, const Color& col) {
  os << "Color(" << col.r << ", " << col.g << ", " << col.b << ", " << col.a << ")";
  return os;
}

} // namespace rei
