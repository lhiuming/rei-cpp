// source of color.h
#include "color.h"

namespace CEL {


// Print
std::ostream& operator<<(std::ostream& os, const Color& col)
{
  os << "Color("
  << col.r << ", " << col.g << ", " << col.b << ", " << col.a
  << ")";
  return os;
}


} // namespace CEL
