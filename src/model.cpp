// source of model.h
#include "model.h"

using namespcae std;

namespace CEL {

ostream& operator<<(ostrem& os, const Vec3 v)
{
  os << "Vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}


}
