#ifndef CEL_ALGEBRA_H
#define CEL_ALGEBRA_H

#include <ostream>
#include <cmath>

/*
 * algebra.h
 * Define some small and useful calss for mathematical computations.
 * TODO: implement Mat4 inverse ?
 * TODO: support move
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
  Vec3() : x(0.0), y(0.0), z(0.0) {};

  // Initialize components
  Vec3(double x, double y, double z) : x(x), y(y), z(z) {};

  // Access elemebt by index (from 0)
  // TODO: but is this portable ??? compliers may have differnt order
  double& operator[](int i) { return (&x)[i]; }
  const double& operator[](int i) const { return (&x)[i]; }

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

  // Normalization
  static void normalize(Vec3& v) { v *= (1.0 / v.norm()); }
  Vec3 normalized() const { return (*this) * (1.0 / this->norm()); }

  // Value check
  bool zero() const { return (x == 0) && (y == 0) && (z == 0); }
  bool operator==(const Vec3& rhs) const {
    return (x == rhs.x) && (y == rhs.y) && (z == rhs.z); }

};

// Scalar multiplications from left
Vec3 operator*(double c, const Vec3& x);

// Dot product
double dot(const Vec3& a, const Vec3& b);

// Corss product
Vec3 cross(const Vec3& a, const Vec3& b);

// Print 3D vector
std::ostream& operator<<(std::ostream& os, const Vec3& v);

// Vec4 ///////////////////////////////////////////////////////////////////////
// A general 4D vector; useful for homogenous coordinates of 3D points.
////

struct Vec4 {
  double x;
  double y;
  double z;
  double h;

  // Default constructor
  Vec4() : x(0.0), y(0.0), z(0.0), h(0.0) {};

  // Initialize with components
  Vec4(double x, double y, double z, double h) : x(x), y(y), z(z), h(h) {};

  // Convert from Vec3
  Vec4(const Vec3& v) : x(v.x), y(v.y), z(v.z), h(0.0) {};
  Vec4(const Vec3& v, double h) : x(v.x), y(v.y), z(v.z), h(h) {};

  // Convert to Vec3
  operator Vec3();

  // Access element by index (from 0)
  double& operator[](int i) { return (&x)[i]; }
  const double& operator[](int i) const { return (&x)[i]; }

  // Scalar multiplications
  Vec4& operator*=(double c) {
    x *= x; y *= c; z *= c; h *= c; return *this; }
  Vec4 operator*(double c) const {
    return Vec4(x * c, y * c, z * c, h * c); }
  Vec4 operator-() const { // negation: -X
    return Vec4(-x, -y, -z, -h); }

  // Vector arithmatics
  Vec4 operator+(const Vec4& rhs) const { // addition
    return Vec4(x + rhs.x, y + rhs.y, z + rhs.z, h + rhs.h); }
  Vec4& operator+=(const Vec4& rhs) {
    x += rhs.x; y += rhs.y; z += rhs.z; h += rhs.h; return *this; }
  Vec4 operator-(const Vec4& rhs) const { // subtraction
    return Vec4(x - rhs.x, y - rhs.y, z - rhs.z, h - rhs.h); }
  Vec4& operator-=(const Vec4& rhs) {
    x -= rhs.x; y -= rhs.y; z -= rhs.z; h -= rhs.h; return *this; }
};

// Scalar multiplications from left
Vec4 operator*(double c, const Vec4& x);

// Dot product
double dot(const Vec4& a, const Vec4& b);

// print 4D vector
std::ostream& operator<<(std::ostream& os, const Vec4& v);

// Mat4 ///////////////////////////////////////////////////////////////////////
// 4x4 matrix. Useful to represent affine transformation in homogenous
// coordinates.
////

class Mat4 {
public:
  // Default constructor (all zero)
  Mat4() {};

  // Initialize with row data; useful for hard-coding constant matrix
  Mat4(const double rows[16]);

  // Construct a diagonal matrix : A(i, i) = diag(i), otherwize zero
  Mat4(const Vec4& diag);

  // Access by column
  Vec4& operator[](int i) { return columns[i]; }
  const Vec4& operator[](int i) const { return columns[i]; }

  // Access by element index (row, col), from 0
  double& operator()(int i, int j) { return columns[j][i]; }
  const double& operator()(int i, int j) const { return columns[j][i]; }

  // Matrix transposition
  static void transpose(Mat4& A);
  Mat4 T() const;

  // Matrix inversion (assuem invertibility)
  static void inverse(Mat4& A);
  Mat4 inv() const;

  // Special matrix
  static Mat4 I(); // identity matrix

  // Matrix multiplication, or transform composition
  Mat4 operator*(const Mat4& rhs) const;

private:
  Vec4 columns[4]; // store data by columns

};

// Print 4D matrix
std::ostream& operator<<(std::ostream& os, const Mat4& m);

// Column-vector transformation : Ax
Vec4 operator*(const Mat4& A, const Vec4& x);

// Row-vector transformation : xA
Vec4 operator*(const Vec4& x, const Mat4& A);

} // namespace CEL

#endif
