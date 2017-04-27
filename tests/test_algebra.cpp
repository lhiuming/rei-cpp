// Test the math model

#include <iostream>
#include <algebra.h>

using namespace std;
using namespace CEL;

int main()
{

  // Test Vec3 //

  // construction
  Vec3 a(11, 1, 1);
  Vec3 b(0, -1, 2.7);
  cout << "a = " << a << endl;
  cout << "b = " << b << endl;

  // scaler multiplication
  a *= 0.3;
  cout << "a *= 0.3; a = " << a << endl;
  cout << "-a = " << -a << endl;
  cout << "a * 3.2 = " << a * 3.2 << endl;

  // arithmatics
  cout << "a + b = " << a + b << endl;
  cout << "a - 2b = " << a - b * 2 << endl;
  a += b;
  b -= a;
  cout << "a += b; a = " << a << endl;
  cout << "b -= a; b = " << b << endl;

  // dot product
  cout << "a .* b = " << dot(a, b) << endl;

  // cross product
  cout << "a x b = " << cross(a, b) << endl;

  // identity matrix
  cout << "Mat4::identity() = " << Mat4::identity() << endl;

  return 0;
}
