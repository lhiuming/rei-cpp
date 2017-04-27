// Test the math model

#include <iostream>
#include <string>
#include <algebra.h>

using namespace std;
using namespace CEL;

void seg(const string s) {
  cout << "--- " << s << " ---" << endl;
}

int main()
{

  // Test Vec3 //
  seg(" >>> Vec3 <<< ");

  seg("construction");
  Vec3 a(1, 2, 3);
  Vec3 b(3, 4, 5);
  cout << "a = " << a << endl;
  cout << "b = " << b << endl;

  seg("scalar multiplication");
  a *= 2;
  cout << "a *= 2; a = " << a << endl;
  cout << "-a = " << -a << endl;
  cout << "a * 3.2 = " << a * 3.2 << endl;

  seg("arithmatics");
  cout << "a + b = " << a + b << endl;
  cout << "a - 2b = " << a - b * 2 << endl;
  a += b;
  b -= a;
  cout << "a += b; a = " << a << endl;
  cout << "b -= a; b = " << b << endl;

  seg("norm and normalization");
  a = Vec3(3, 0, 4);
  cout << "a = " << a << " norm(a) = " << a.norm() << endl;
  cout << "a.normalized() == " << a.normalized() << endl;

  seg("value check");
  cout << "a == Vec3(3, 0, 4): " << (int)(a == Vec3(3, 0, 4)) << endl;
  cout << "a == Vec3(3, 4, 0): " << (int)(a == Vec3(3, 4, 0)) << endl;
  cout << "a.zero() == " << (int)(a.zero()) << endl;

  seg("dot and cross product");
  cout << "a = " << a << ", b = " << b << endl;
  cout << "a .* b = " << dot(a, b) << endl;
  cout << "a x b = " << cross(a, b) << endl;

  // Test Vec4 andt Mat4
  seg(" >>> Vec4 and Mat4 <<< ");

  seg("construction");
  Vec4 x(0, 1, 0, -1);
  Vec4 y(1, 0, 0,  1);
  Vec4 z(0, 0, 5,  0);
  Vec4 h(0, 0, 0,  1);
  Mat4 A; A[0] = x; A[1] = y; A[2] = z; A[3] = h;
  cout << "x = " << x << endl;
  cout << "A = " << A << endl;

  seg("transpose");
  cout << "A' = " << A.T() << endl;

  seg("special matrix");
  cout << "Mat4::I() = " << Mat4::I() << endl;

  seg("transform");
  cout << "A x = " << A*x << endl;

  seg("composition");
  cout << "A * I = " << A * Mat4::I() << endl;
  cout << "A * A = " << A * A << endl;

  return 0;
}
