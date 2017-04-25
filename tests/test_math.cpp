// Test the math model

#include <iostream>
#include <math.h>

using namespace std;
using namespace CEL;

int main()
{
  // construction
  Vec3 a(11, 1, 1);
  Vec3 b(0, -1, 2.7);
  cout << "a = " << a << endl;
  cout << "b = " << b << endl;

  // scaler multiply
  cout << "a * 3.2 = " << a * 3.2 << endl;

  // addition
  cout << "a + b = " << a + b << endl;
  cout << "a - 2b = " << a - b * 2 << endl;

  // dot product
  cout << "a .* b = " << dot(a, b) << endl;

  return 0;
}
