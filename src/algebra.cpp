// source of math.h
#include "algebra.h"

#include "console.h"

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


// Mat3 //////////////////////////////////////////////////////////////////////
// Member and Non-member functions.
////

// Print Mat3
std::ostream& operator<<(std::ostream& os, const Mat3& m)
{
  os << "Mat3[" << endl;
  for (int i = 0; i < 2; ++i) {
    os << "(";
    for (int j = 0; j < 2; ++j) os << m(i, j) << ", ";
    os << m(i, 2) << ")," << endl;
  }
  os << "(";
  for (int j = 0; j < 2; ++j) os << m(2, j) << ", ";
  os << m(2, 2) << ")" << "]";
  return os;
}


// Vec4 //////////////////////////////////////////////////////////////////////
// Non-members.
////

// Scalar multiplications from left
Vec4 operator*(double c, const Vec4& x)
{
  return x * c;
}

// Dot product
double dot(const Vec4& a, const Vec4& b)
{
  return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.h * b.h);
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
Mat4::Mat4(const double rows[16])
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
void Mat4::transpose(Mat4& A)
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

// Determinant
double Mat4::det() const
{
  // Laplacian expansion
  return dot(
    columns[0],
    Vec4(cofactor(0, 0), cofactor(1, 0), cofactor(2, 0), cofactor(3, 0))
  );
}

// Inverse a matrix
void Mat4::inverse(Mat4& A)
{
  // TODO : better implementation
  A = A.inv();
}

// Inversed version of self
Mat4 Mat4::inv() const
{
  // Kind of low efficiency
  return (1.0 / det()) * adjoint();
}

// Matrix multiplication, or transform composition
Mat4 Mat4::operator*(const Mat4& rhs) const
{
  const Mat4& me = *this;
  Mat4 ret;
  ret[0] = me * rhs[0];
  ret[1] = me * rhs[1];
  ret[2] = me * rhs[2];
  ret[3] = me * rhs[3];
  return ret;
}

// Upper-left 3x3
Mat3 Mat4::sub3() const
{
  return Mat3(
    columns[0].truncated(),
    columns[1].truncated(),
    columns[2].truncated());
}

// Adjoint of the upper-left 3x3 matrix; is transpose of co-factor matrix
Mat3 Mat4::adj3() const
{
  const Mat4& A = *this;
  return Mat3(
    A(1,1)*A(2,2)-A(1,2)*A(2,1), A(2,1)*A(0,2)-A(2,2)*A(0,1),
    A(0,1)*A(1,2)-A(0,2)*A(1,1),
    A(1,2)*A(2,0)-A(1,0)*A(2,2), A(2,2)*A(0,0)-A(2,0)*A(0,2),
    A(0,2)*A(1,0)-A(0,0)*A(1,2),
    A(1,0)*A(2,1)-A(1,1)*A(2,0), A(2,0)*A(0,1)-A(2,1)*A(0,0),
    A(0,0)*A(1,1)-A(0,1)*A(1,0)
  );
}

// Minor (reduced determinant)
double Mat4::minor(int i, int j) const
{
  // Now I really wish to learn meta-programming ... this is UGLY
  auto A = [=](int r, int c) -> double {
    return (*this)( (r >= i) ? (r + 1) : r,  (c >= j) ? (c + 1) : c );
  };
  return A(0,0) * ( A(1,1) * A(2,2) - A(1,2) * A(2,1) )
       + A(1,0) * ( A(2,1) * A(0,2) - A(2,2) * A(0,1) )
       + A(2,0) * ( A(0,1) * A(1,2) - A(0,2) * A(1,1) );
}

// Cofactor (signed minor)
double Mat4::cofactor(int i, int j) const
{
  // return pow(-1, i+j) * minor(i, j)
  if ((i+j) & 1) return -1 * minor(i, j);
  return minor(i, j);
}

// Adjoint Matrix 4D
Mat4 Mat4::adjoint() const
{
  // NOTE : simple transpose of cofactor matrix
  return Mat4(
    cofactor(0,0), cofactor(1,0), cofactor(2,0), cofactor(3,0),
    cofactor(0,1), cofactor(1,1), cofactor(2,1), cofactor(3,1),
    cofactor(0,2), cofactor(1,2), cofactor(2,2), cofactor(3,2),
    cofactor(0,3), cofactor(1,3), cofactor(2,3), cofactor(3,3)
  );
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


} // namespace CEL
