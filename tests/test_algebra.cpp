// Test the math model

#include <algebra.h>
#include <console.h>
#include <iostream>
#include <string>

using namespace std;
using namespace CEL;

void seg(const string s) {
  console << "--- " << s << " ---" << endl;
}

int main() {
  // Test Vec3 //
  seg(" >>> Vec3 <<< ");

  seg("construction");
  Vec3 a(1, 2, 3);
  Vec3 b(3, 4, 5);
  console << "a = " << a << endl;
  console << "b = " << b << endl;

  seg("scalar multiplication");
  a *= 2;
  console << "a *= 2; a = " << a << endl;
  console << "-a = " << -a << endl;
  console << "a * 3.2 = " << a * 3.2 << endl;

  seg("arithmatics");
  console << "a + b = " << a + b << endl;
  console << "a - 2b = " << a - b * 2 << endl;
  a += b;
  b -= a;
  console << "a += b; a = " << a << endl;
  console << "b -= a; b = " << b << endl;

  seg("norm and normalization");
  a = Vec3(3, 0, 4);
  console << "a = " << a << " norm(a) = " << a.norm() << endl;
  console << "a.normalized() == " << a.normalized() << endl;

  seg("value check");
  console << "a == Vec3(3, 0, 4): " << (int)(a == Vec3(3, 0, 4)) << endl;
  console << "a == Vec3(3, 4, 0): " << (int)(a == Vec3(3, 4, 0)) << endl;
  console << "a.zero() == " << (int)(a.zero()) << endl;

  seg("dot and cross product");
  console << "a = " << a << ", b = " << b << endl;
  console << "a .* b = " << dot(a, b) << endl;
  console << "a x b = " << cross(a, b) << endl;

  // Test Vec4 andt Mat4
  seg(" >>> Vec4 and Mat4 <<< ");

  seg("construction");
  Vec4 x(0, 1, 0, -1);
  Vec4 y(1, 0, 0, 1);
  Vec4 z(0, 0, 5, 0);
  Vec4 h(0, 0, 0, 1);
  Mat4 A;
  A[0] = x;
  A[1] = y;
  A[2] = z;
  A[3] = h;
  console << "x = " << x << endl;
  console << "A = " << A << endl;

  seg("transpose");
  console << "A' = " << A.T() << endl;

  seg("special matrix");
  console << "Mat4::I() = " << Mat4::I() << endl;

  seg("transform");
  console << "A x = " << A * x << endl;
  console << "x A = " << x * A << endl;

  seg("composition");
  console << "A * I = " << A * Mat4::I() << endl;
  console << "A * A = " << A * A << endl;

  seg("inversion");
  console << "A.minor(0, 1) = " << A.minor(0, 1) << endl;
  console << "A.cofactor(0, 1) = " << A.cofactor(0, 1) << endl;
  console << "A.inv() = " << A.inv() << endl;
  console << "A * A.inv() = " << A * A.inv() << endl;

  return 0;
}
