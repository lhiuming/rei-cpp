#ifndef CEL_ALGEBRA_H
#define CEL_ALGEBRA_H

#include <ostream>
#include <cmath>

/*
 * math.h
 * Define some small and useful calss for mathematical computations.
 * TODO: Vec3 cross products
 * TODO: matrix 4 operations
 * TODO: support move to same copying
 */

namespace CEL {

// Vec3 //////////////////////////////////////////////////////////////////////
// A general 3D vector class
////

struct Vec3 {
  double x;
  double y;
  double z;

  // Default constructor
  Vec3() {};

  // Initialize components
  Vec3(double x, double y, double z) : x(x), y(y), z(z) {};

  // Scalar multiplications
  Vec3& operator*=(double c) {
    x *= x; y *= c; z *= c; return *this; }
  Vec3 operator*(double c) const {
    return Vec3(x * c, y * c, z * c); }
  Vec3 operator-() const { // negation: -X
    return Vec3(-x, -y, -z); }

  // Vector arithmatics
  Vec3 operator+(const Vec3& rhs) const { // addition
    return Vec3(x + rhs.x, y + rhs.y, z + rhs.z); }
  Vec3& operator+=(const Vec3& rhs) {
    x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
  Vec3 operator-(const Vec3& rhs) const { // subtraction
    return Vec3(x - rhs.x, y - rhs.y, z - rhs.z); }
  Vec3& operator-=(const Vec3& rhs) {
    x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }

  // Nroms
  double norm2() const { return x*x + y*y + z*z; }
  double norm() const { return std::sqrt(norm2()); }

  // normalization
  void normalize() {
    double r = 1.0 / this->norm();
    x *= r; y *= r; z *= r;
  }
  Vec3 normalized() const {
    double r = 1.0 / this->norm();
    return Vec3(x * r, y * r, z * r);
  }

  // value check
  bool zero() const { return (x == 0) && (y == 0) && (z == 0); }

};

// dot product
double dot(const Vec3& a, const Vec3& b);

// cross product
Vec3 cross(const Vec3& a, const Vec3& b);

// print 3D vector
std::ostream& operator<<(std::ostream& os, const Vec3& v);


// 4D vector //////////////////////////////////////////////////////////////////
struct Vec4 {
  double x;
  double y;
  double z;
  double h;

  Vec4() {};
  Vec4(double x, double y, double z, double h = 1.0)
    : x(x), y(y), z(z), h(h) {};

  // convert from Vec3
  Vec4(Vec3& v, double h = 1.0) : x(v.x), y(v.y), z(v.z), h(h) {};
  Vec4(Vec3&& v, double h = 1.0) : x(v.x), y(v.y), z(v.z), h(h) {};

  // access elements
  double& operator[](int i) { return (&x)[i]; }
  const double& operator[](int i) const { return (&x)[i]; }

};

// print 4D vector
std::ostream& operator<<(std::ostream& os, const Vec4& v);


// 4x4 Matrix /////////////////////////////////////////////////////////////////
struct Mat4 {

  // default constructor; all zero matrix
  Mat4() {};

  // row data constructor; useful for constexpr
  Mat4(double rows[16]);

  // access by columns
  Vec4& operator[](int i) { return columns[i]; }
  const Vec4& operator[](int i) const { return columns[i]; }

  // access by element (row, col), indexed from 0
  double& operator()(int i, int j) { return columns[j][i]; }
  const double& operator()(int i, int j) const { return columns[j][i]; }

  // useful operations
  Mat4 transposed() const;

  // useful matrix
  static Mat4 identity();

private:
  Vec4 columns[4]; // store data by columns

};

// print 4D matrix
std::ostream& operator<<(std::ostream& os, const Mat4& m);


} // namespace CEL

#endif
