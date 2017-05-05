// Test the color lib

#include <iostream>
#include <color.h>

using namespace std;
using namespace CEL;

int main()
{
  Color c(0.2, 0.3, 0.1);
  cout << "c = " << c << endl;
  cout << "c * 0.3 = " << c * 0.3 << endl;

  return 0;
}
