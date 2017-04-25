#ifndef CEL_MATH_H
#define CEL_MATH_H

#include <ostream>

/*
 * math.h
 * Define some small and useful calss for mathematical computations.
 * TODO: Vec3 cross products 
 * TODO: matrix 4 operations
 */

namespace CEL {

// 3D vector //////////////////////////////////////////////////////////////////
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

// print 3D vector
std::ostream& operator<<(std::ostream& os, const Vec3& v);


// 4D vector //////////////////////////////////////////////////////////////////
struct Vec4 {
  double x;
  double y;
  double z;
  double h;

  Vec4(double x, double y, double z, double h = 1.0)
    : x(x), y(y), z(z), h(h) {};

  // convert from Vec3
  Vec4(Vec3& v, double h = 1.0) : x(v.x), y(v.y), z(v.z), h(h) {};
  Vec4(Vec3&& v, double h = 1.0) : x(v.x), y(v.y), z(v.z), h(h) {};
};

// print 4D vector
std::ostream& operator<<(std::ostream& os, const Vec4& v);


// 4x4 Matrix /////////////////////////////////////////////////////////////////
class Mat4 {
public:
  Mat4() = default;

private:
  double data[16]; // NOTE: shoule be accessed column wise

};

} // namespace CEL

#endif
