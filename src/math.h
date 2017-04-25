#ifndef CEL_MATH_H
#define CEL_MATH_H

#include <ostream>

/*
 * math.h
 * Define some small and useful calss for mathematical computations.
 */

namespace CEL {

// 3D vector
struct Vec3 {
  double x;
  double y;
  double z;

  Vec3(double x, double y, double z) : x(x), y(y), z(z) {};

  // scalar multiplication
  Vec3 operator*(const double c) const {
    return Vec3(x * c, y * c, z * c);
  }

  // vector addition
  Vec3 operator+(const Vec3& rhs) const {
    return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
  }
  Vec3 operator-(const Vec3& rhs) const {
    return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
  }

};

// dot product
double dot(const Vec3& a, const Vec3& b);

// cross product
// TODO: implement me
Vec3 cross(const Vec3& a, const Vec3& b);

// print the vector
std::ostream& operator<<(std::ostream& os, const Vec3& v);

} // namespace CEL

#endif
