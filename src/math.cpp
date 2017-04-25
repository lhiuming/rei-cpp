// source of math.h
#include "math.h"

using namespace std;

namespace CEL {

// print
ostream& operator<<(ostream& os, const Vec3& v)
{
  os << "Vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

// dot product
double dot(const Vec3& a, const Vec3& b)
{
  return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

// cross product : TODO 

} // namespace CEL
