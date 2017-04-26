// source of math.h
#include "math.h"

using namespace std;

namespace CEL {

// print Vec3
ostream& operator<<(ostream& os, const Vec3& v)
{
  os << "Vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

// vec3 dot product
double dot(const Vec3& a, const Vec3& b)
{
  return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

// Vec3 cross product
Vec3 cross(const Vec3& a, const Vec3& b)
{
  return Vec3(a.y * b.z - a.z * b.y,
              a.z * b.x - a.x * b.z,
              a.x * b.y - a.y * b.x);
}

// print Vec4

ostream& operator<<(ostream& os, const Vec4& v)
{
  os << "Vec4(" << v.x << ", " << v.y << ", " << v.z << ", " << v.h <<")";
  return os;
}

} // namespace CEL
