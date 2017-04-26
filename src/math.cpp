// source of math.h
#include "math.h"
#include <iostream>

using namespace std;

namespace CEL {

// Vec //////////////////////////////////////////////////////////////////////

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


// Mat4 //////////////////////////////////////////////////////////////////////

Mat4::Mat4(double rows[16])
{
  Mat4& me = *this;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      me(i, j) = rows[i * 4 + j];
}

// identity matrix
Mat4 Mat4::identity()
{
  Mat4 ret;
  for (int i = 0; i < 4; ++i)
    ret(i, i) = 1.0;
  return ret;
}

// transpose
Mat4 Mat4::transposed() const
{
  const Mat4& me = *this;
  Mat4 ret;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
    {
      ret(i, j) = me(j, i);
    }
  return ret;
}

// print Mat4
std::ostream& operator<<(std::ostream& os, const Mat4& m)
{
  Mat4 t = m.transposed();
  os << "Mat4[" << t[0] << endl;
  os << " " << t[1] << endl;
  os << " " << t[2] << endl;
  os << " " << t[3] << "]" ;
  return os;
}


} // namespace CEL
