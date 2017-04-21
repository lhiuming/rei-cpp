// Test the OpenGL API wrapper (pixels.h)

#include <iostream>

#include <pixels.h>

using namespace std;

int main()
{
  int suc = CEL::gl_init(720, 640, "test_pixels.cpp");
  cout << "Successfully call gl_init()" << endl;

  return 0;
}
