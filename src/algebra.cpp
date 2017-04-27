// source of math.h
#include "algebra.h"

#include <iostream>

using namespace std;

namespace CEL {

// Vec3 ///////////////////////////////////////////////////////////////////////
////

// Scalar multiplications from left
Vec3 operator*(double c, const Vec3& x)
{
  return x * c;
}

// Vec3 dot product
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

// Print Vec3
ostream& operator<<(ostream& os, const Vec3& v)
{
  os << "Vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

// Vec4 //////////////////////////////////////////////////////////////////////
////

// Scalar multiplications from left
Vec4 operator*(double c, const Vec4& x)
{
  return x * c;
}

// Print Vec4
ostream& operator<<(ostream& os, const Vec4& v)
{
  os << "Vec4(" << v.x << ", " << v.y << ", " << v.z << ", " << v.h <<")";
  return os;
}


// Mat4 //////////////////////////////////////////////////////////////////////
////

// Row data constructor
Mat4::Mat4(double rows[16])
{
  Mat4& me = *this;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      me(i, j) = rows[i * 4 + j];
}

// Diagonal constructor
Mat4::Mat4(const Vec4& diag)
{
  columns[0][0] = diag[0]; columns[1][1] = diag[1];
  columns[2][2] = diag[2]; columns[3][3] = diag[3];
}


// Transpose a matrix
void transpose(Mat4& A)
{
  for (int i = 0; i < 3; ++i)
    for (int j = i + 1; j < 3; ++j)
    {
      double t = A(i, j);
      A(i, j) = A(j, i);
      A(j, i) = t;
    }
}

// Transposed version of self
Mat4 Mat4::T() const
{
  const Mat4& me = *this;
  Mat4 ret;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      ret(i, j) = me(j, i);
  return ret;
}

// Inverse a matrix
void inverse(Mat4& A)
{
  A = A.inv();
}

// Inversed version of self
Mat4 Mat4::inv() const
{
  // TODO: implement me ?
  cerr << "Warm: used a un-implemented method: Mat4::inv" << endl;
  return I();
}

// Make Identity matrix
Mat4 Mat4::I()
{
  return Mat4(Vec4(1.0, 1.0, 1.0, 1.0));
}

// Print Mat4
std::ostream& operator<<(std::ostream& os, const Mat4& m)
{
  os << "Mat4[" << endl;
  for (int i = 0; i < 3; ++i) {
    os << "(";
    for (int j = 0; j < 3; ++j) os << m(i, j) << ", ";
    os << m(i, 3) << ")," << endl;
  }
  os << "(";
  for (int j = 0; j < 3; ++j) os << m(3, j) << ", ";
  os << m(3, 3) << ")" << "]";
  return os;
}

// Vector transformation : Ax
Vec4 operator*(const Mat4& A, const Vec4& x)
{ // linear combination of columns
  return x[0] * A[0] +
         x[1] * A[1] +
         x[2] * A[2] +
         x[3] * A[3];
}

} // namespace CEL
