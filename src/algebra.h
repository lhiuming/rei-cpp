#ifndef REI_ALGEBRA_H
#define REI_ALGEBRA_H

#include <cmath>
#include <ostream>

/*
 * algebra.h
 * Define some small and useful calss for mathematical computations.
 *
 * Both Mat3 and Mat4 use column-major storage.
 *
 * TODO: implement Mat4 inverse ?
 * TODO: support move
 */

namespace rei {

// Numerics type used in REI
typedef float real_t;

// Typed constants for in-source documentation

// Indicating the handness of intented coordinate space. Default should be Right.
enum class Handness : unsigned char { Right, Left };
// Indicating the usage of transform matrix.
// For example, matrix targeting column-vector should be used in the form `M*v`.
// Column vector as default, to match whth column-major storage.
enum class VectorTarget : unsigned char { Column, Row };

// Vec2 //////////////////////////////////////////////////////////////////////
// A general 2D vector class
////

struct Vec2;
inline Vec2 operator*(real_t c, const Vec2& v);
inline Vec2 operator*(const Vec2& v, const Vec2& c);
inline real_t dot(const Vec2& a, const Vec2& b);

struct Vec2 {
  real_t x;
  real_t y;

  constexpr Vec2() : x(0), y(0) {};
  constexpr Vec2(real_t x, real_t y) : x(x), y(y) {};

  real_t& operator[](int i) { return (&x)[i]; }
  const real_t& operator[](int i) const { return (&x)[i]; }

  // Scalar multiplications
  Vec2& operator*=(real_t c) {
    x *= c;
    y *= c;
    return *this;
  }
  Vec2 operator*(real_t c) const { return Vec2(x * c, y * c); }
  Vec2 operator-() const { return Vec2(-x, -y); }

  // Vector arithmatics
  Vec2 operator+(const Vec2& rhs) const { // addition
    return Vec2(x + rhs.x, y + rhs.y);
  }
  Vec2& operator+=(const Vec2& rhs) {
    x += rhs.x;
    y += rhs.y;
    return *this;
  }
  Vec2 operator-(const Vec2& rhs) const { // subtraction
    return Vec2(x - rhs.x, y - rhs.y);
  }
  Vec2& operator-=(const Vec2& rhs) {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
  }

  // Nroms
  real_t norm2() const { return x * x + y * y; }
  real_t norm() const { return std::sqrt(norm2()); }

  // Normalization
  static inline void normalize(Vec2& v) { v *= (real_t(1) / v.norm()); }
  Vec2 normalized() const { return (*this) * (real_t(1) / this->norm()); }

  // Value check
  bool is_zero() const { return (x == real_t(0)) && (y == real_t(0)); }
  bool operator==(const Vec2& rhs) const { return (x == rhs.x) && (y == rhs.y); }
};

inline Vec2 operator*(real_t c, const Vec2& v) {
  return Vec2(c * v.x, c * v.y);
}
inline Vec2 operator*(const Vec2& a, const Vec2& b) {
  return Vec2(a.x * b.x, a.y * b.y);
}
inline real_t dot(const Vec2& a, const Vec2& b) {
  return a.x * b.x + a.y * b.y;
}

// Print 2D vector
std::wostream& operator<<(std::wostream& os, const Vec2& v);

// Vec3 //////////////////////////////////////////////////////////////////////
// A general 3D vector class
////

struct Vec3;
inline Vec3 operator*(real_t c, const Vec3& x);
inline Vec3 operator*(const Vec3& a, const Vec3& b);
inline real_t dot(const Vec3& a, const Vec3& b);
inline Vec3 cross(const Vec3& a, const Vec3& b);

struct Vec3 {
  real_t x;
  real_t y;
  real_t z;

  // Default constructor
  constexpr Vec3() : x(0), y(0), z(0) {};

  // Initialize components
  constexpr Vec3(real_t x, real_t y, real_t z) : x(x), y(y), z(z) {};

  // Access elemebt by index (from 0)
  real_t& operator[](int i) { return (&x)[i]; }
  const real_t& operator[](int i) const { return (&x)[i]; }

  // Scalar multiplications
  Vec3& operator*=(real_t c) {
    x *= c;
    y *= c;
    z *= c;
    return *this;
  }
  Vec3 operator*(real_t c) const { return Vec3(x * c, y * c, z * c); }
  Vec3 operator-() const { return Vec3(-x, -y, -z); }

  // Vector arithmatics
  Vec3 operator+(const Vec3& rhs) const { // addition
    return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
  }
  Vec3& operator+=(const Vec3& rhs) {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
  }
  Vec3 operator-(const Vec3& rhs) const { // subtraction
    return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
  }
  Vec3& operator-=(const Vec3& rhs) {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
  }

  // Nroms
  real_t norm2() const { return x * x + y * y + z * z; }
  real_t norm() const { return std::sqrt(norm2()); }

  // Normalization
  static inline void normalize(Vec3& v) { v *= (real_t(1) / v.norm()); }
  Vec3 normalized() const { return (*this) * (real_t(1) / this->norm()); }

  // transforms
  static inline void rotate(Vec3& v, const Vec3& axis, real_t radian) {
    real_t c = std::cos(radian), c_cp = real_t(1) - c, s = std::sin(radian);
    Vec3 u = dot(v, axis) * axis, r = v - u;
    v = u + s * cross(axis, r) + c * r;
  }
  Vec3 rotated(const Vec3& axis, real_t radian) const {
    Vec3 ret = *this;
    rotate(ret, axis, radian);
    return ret;
  }

  // Value check
  bool is_zero() const { return (x == real_t(0)) && (y == real_t(0)) && (z == real_t(0)); }
  bool operator==(const Vec3& rhs) const { return (x == rhs.x) && (y == rhs.y) && (z == rhs.z); }
};

// Scalar multiplications from left
inline Vec3 operator*(real_t c, const Vec3& x) {
  return x * c;
}

// Element-wise mitiplication
inline Vec3 operator*(const Vec3& a, const Vec3& b) {
  return {a.x * b.x, a.y * b.y, a.z * b.z};
}

inline real_t dot(const Vec3& a, const Vec3& b) {
  return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

// Vec3 cross product
inline Vec3 cross(const Vec3& a, const Vec3& b) {
  return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

// Common data transform
inline void flip_z(Vec3& v) {
  v.z = -v.z;
}

// Print 3D vector
std::wostream& operator<<(std::wostream& os, const Vec3& v);

// Mat3 ///////////////////////////////////////////////////////////////////////
// 3x3 matrix. Very limited.
////

struct Mat3 {
  Vec3 columns[3];

  // Default constructor
  Mat3() {};

  // Construct by columns
  Mat3(const Vec3& c1, const Vec3& c2, const Vec3& c3) : columns {c1, c2, c3} {}
  Mat3(Vec3&& c1, Vec3&& c2, Vec3&& c3) : columns {c1, c2, c3} {}

  // Initialize with row data; useful for hard-coding constant matrix
  Mat3(const real_t rows[9]); // TODO
  Mat3(real_t a00, real_t a01, real_t a02, real_t a10, real_t a11, real_t a12, real_t a20, real_t a21, real_t a22)
      : columns {{a00, a10, a20}, {a01, a11, a21}, {a02, a12, a22}} {}

  // Construct a diagonal matrix : A(i, i) = diag(i), otherwize zero
  Mat3(const Vec3& diag); // TODO

  // Access by column
  Vec3& operator[](int i) { return columns[i]; }
  const Vec3& operator[](int i) const { return columns[i]; }

  // Access by element index (row, col), from 0
  real_t& operator()(int i, int j) { return columns[j][i]; }
  const real_t& operator()(int i, int j) const { return columns[j][i]; }

  // Scalar multiplication
  Mat3& operator*=(real_t c) {
    columns[0] *= c;
    columns[1] *= c;
    columns[2] *= c;
    return *this;
  }
  Mat3 operator*(real_t c) const {
    Mat3 ret {*this};
    return ret *= c;
  }

  // Matrix transposition
  static void transpose(Mat3& A); // TODO
  Mat3 T() const;                 // TODO

  // Determinant
  real_t det() const;

  // Matrix inversion (assuem invertibility)
  static void inverse(Mat3& A); // TODO
  Mat3 inv() const;             // TODO
};

// Print 3D matrix
std::wostream& operator<<(std::wostream& os, const Mat3& m); // TODO

// Scalar multiplication from left
inline Mat3 operator*(real_t c, const Mat3& A) {
  return A * c;
}

// Column-vector transformation : Ax
Vec3 operator*(const Mat3& A, const Vec3& x); // TODO

// Row-vector transformation : xA
inline Vec3 operator*(const Vec3& x, const Mat3& A) {
  return Vec3(dot(x, A[0]), dot(x, A[1]), dot(x, A[2]));
}

// Vec4 ///////////////////////////////////////////////////////////////////////
// A general 4D vector; useful for homogenous coordinates of 3D points.
////

struct Vec4 {
  real_t x;
  real_t y;
  real_t z;
  real_t h;

  // Default constructor
  constexpr Vec4() : x(0), y(0), z(0), h(0) {};

  // Initialize with components
  constexpr Vec4(real_t x, real_t y, real_t z, real_t h) : x(x), y(y), z(z), h(h) {};
  constexpr Vec4(const Vec3& v, real_t h) : x(v.x), y(v.y), z(v.z), h(h) {};

  // Access element by index (from 0)
  real_t& operator[](int i) { return (&x)[i]; }
  const real_t& operator[](int i) const { return (&x)[i]; }

  // Scalar multiplications
  Vec4& operator*=(real_t c) {
    x *= x;
    y *= c;
    z *= c;
    h *= c;
    return *this;
  }
  Vec4 operator*(real_t c) const { return Vec4(x * c, y * c, z * c, h * c); }
  Vec4 operator-() const { // negation: -X
    return Vec4(-x, -y, -z, -h);
  }

  // Vector arithmatics
  Vec4 operator+(const Vec4& rhs) const { // addition
    return Vec4(x + rhs.x, y + rhs.y, z + rhs.z, h + rhs.h);
  }
  Vec4& operator+=(const Vec4& rhs) {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    h += rhs.h;
    return *this;
  }
  Vec4 operator-(const Vec4& rhs) const { // subtraction
    return Vec4(x - rhs.x, y - rhs.y, z - rhs.z, h - rhs.h);
  }
  Vec4& operator-=(const Vec4& rhs) {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    h -= rhs.h;
    return *this;
  }

  // Convert to Vec3 from 4Dhomogenous, or projection/truncating
  Vec3 homo_div() const { return Vec3(x / h, y / h, z / h); }
  Vec3 truncated() const { return Vec3(x, y, z); }
};

// print 4D vector
std::wostream& operator<<(std::wostream& os, const Vec4& v);

// Scalar multiplications from left
Vec4 operator*(real_t c, const Vec4& x);

// Dot product
real_t dot(const Vec4& a, const Vec4& b);

// Mat4 ///////////////////////////////////////////////////////////////////////
// 4x4 matrix. Useful to represent affine transformation in homogenous
// coordinates.
////

struct Mat4 {
  Vec4 columns[4]; // column-major storage

  // Default constructor (all zero)
  constexpr Mat4() {};

  // Construct by columns
  constexpr Mat4(const Vec4& c1, const Vec4& c2, const Vec4& c3, const Vec4& c4)
      : columns {c1, c2, c3, c4} {}
  // constexpr Mat4(Vec4&& c1, Vec4&& c2, Vec4&& c3, Vec4&& c4) : columns {c1, c2, c3, c4} {}

  // Initialize with row data; useful for hard-coding constant matrix
  Mat4(const real_t rows[16]);
  constexpr Mat4(real_t a00, real_t a01, real_t a02, real_t a03, real_t a10, real_t a11, real_t a12, real_t a13,
    real_t a20, real_t a21, real_t a22, real_t a23, real_t a30, real_t a31, real_t a32, real_t a33)
      : columns {Vec4(a00, a10, a20, a30), Vec4(a01, a11, a21, a31), Vec4(a02, a12, a22, a32),
        Vec4(a03, a13, a23, a33)} {}

  // Construct a diagonal matrix : A(i, i) = diag(i), otherwize zero
  constexpr static Mat4 from_diag(const Vec4& diag) {
    return {
      {diag.x, 0, 0, 0},
      {0, diag.y, 0, 0},
      {0, 0, diag.z, 0},
      {0, 0, 0, diag.h},
    };
  }

  // Construct a Identity matrix
  constexpr static Mat4 I() { return from_diag({1, 1, 1, 1}); }

  // Construct a translation matrix
  constexpr static Mat4 translate(
    const Vec3& translate, VectorTarget target = VectorTarget::Column) {
    return {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {translate, 1}};
  }

  // Construct a rotation matrix
  static Mat4 rotate(const Vec3& axis, real_t radian, Handness rot_hand = Handness::Right) {
    real_t s = std::sin(radian), c = std::cos(radian), c_cp = real_t(1) - c;
    Vec4 c0 = {c_cp * axis.x * axis + Vec3(1, axis.z, -axis.y) * Vec3(c, s, s), 0};
    Vec4 c1 = {c_cp * axis.y * axis + Vec3(-axis.z, 1, axis.x) * Vec3(s, c, s), 0};
    Vec4 c2 = {c_cp * axis.z * axis + Vec3(axis.y, -axis.x, 1) * Vec3(s, s, c), 0};
    Vec4 c3 = {0, 0, 0, 1};
    return {c0, c1, c2, c3};
  }

  // Construct a translation and rotation matrix (translate first, then rotate locally)
  static Mat4 translate_rotate(
    const Vec3& translate, const Vec3& axis, real_t radian, Handness rot_hand = Handness::Right) {
    real_t s = std::sin(radian), c = std::cos(radian), c_cp = real_t(1) - c;
    Vec4 c0 = {c_cp * axis.x * axis + Vec3(1, axis.z, -axis.y) * Vec3(c, s, s), 0};
    Vec4 c1 = {c_cp * axis.y * axis + Vec3(-axis.z, 1, axis.x) * Vec3(s, c, s), 0};
    Vec4 c2 = {c_cp * axis.z * axis + Vec3(axis.y, -axis.x, 1) * Vec3(s, s, c), 0};
    Vec4 c3 = {translate, 1};
    return {c0, c1, c2, c3};
  }

  // Access by column
  Vec4& operator[](int i) { return columns[i]; }
  const Vec4& operator[](int i) const { return columns[i]; }

  // Access by element index (row, col), from 0
  real_t& operator()(int i, int j) { return columns[j][i]; }
  const real_t& operator()(int i, int j) const { return columns[j][i]; }

  // Element-wise operations
  Mat4 operator-(const Mat4& rhs) const;

  // Matrix transposition
  static void transpose(Mat4& A);
  Mat4 T() const;

  // Matric determinant (by Laplacian expansion)
  real_t det() const;

  // Matrix inversion (assuem invertibility)
  static void inverse(Mat4& A);
  Mat4 inv() const;

  // Matrix multiplication, or transform composition
  Mat4 operator*(const Mat4& rhs) const;

  // Upper-left 3x3 sub-matrix
  Mat3 sub3() const;

  // Adjoint of the upper-left 3x3 matrix; useful for normal transform
  Mat3 adj3() const;

  // Minor (reduced determinant)
  real_t minor(int i, int j) const;

  // Cofactor (signed minor)
  real_t cofactor(int i, int j) const;

  // Adjoint Mat4
  Mat4 adjoint() const;

  // Frobenius norm
  real_t norm2() const;
  real_t norm() const { return std::sqrt(norm2()); }
};

// Print 4D matrix
std::wostream& operator<<(std::wostream& os, const Mat4& m);

// Scalar multiplication
inline Mat4 operator*(const Mat4& A, real_t c) {
  return Mat4(A[0] * c, A[1] * c, A[2] * c, A[3] * c);
}
inline Mat4 operator*(real_t c, const Mat4& A) {
  return A * c;
}

// Column-vector transformation : Ax
inline Vec4 operator*(const Mat4& A, const Vec4& x) { // linear combination of columns
  return x[0] * A[0] + x[1] * A[1] + x[2] * A[2] + x[3] * A[3];
}

// Row-vector transformation : xA
inline Vec4 operator*(const Vec4& x, const Mat4& A) {
  return Vec4(dot(x, A[0]), dot(x, A[1]), dot(x, A[2]), dot(x, A[3]));
}

// Common data transform
inline void flip_z_column(Mat4& m) {
  m[2] = -m[2];
}

inline void flip_z_row(Mat4& m) {
  m(2, 0) *= real_t(-1);
  m(2, 1) *= real_t(-1);
  m(2, 2) *= real_t(-1);
  m(2, 3) *= real_t(-1);
}

inline void handness_convert(Mat4& mat, bool rhs_handness_flip, bool lhs_handness_flip) {
  if (rhs_handness_flip) { flip_z_column(mat); }
  if (lhs_handness_flip) { flip_z_row(mat); }
}

inline void convention_convert(
  Mat4& mat, bool rhs_handness_flip, bool lhs_handness_flip, bool target_flip) {
  handness_convert(mat, rhs_handness_flip, lhs_handness_flip);
  if (target_flip) Mat4::transpose(mat);
}
inline Mat4 convention_convert(
  const Mat4& mat, bool rhs_handness_flip, bool lhs_handness_flip, bool target_flip) {
  Mat4 ret = mat;
  handness_convert(ret, rhs_handness_flip, lhs_handness_flip);
  return target_flip ? ret.T() : ret;
}

} // namespace rei

#endif
