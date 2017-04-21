// Test the OpenGL API wrapper (pixels.h)
// Create a window and draw a spining dot

#include <cmath>

#include <iostream>
#include <vector>

#include <pixels.h>

//#define DEBUG_PRINT

using namespace std;
using namespace CEL;

int main()
{
  if (gl_init(720, 480, "test_pixels.cpp"))
  {
    cerr << "Call gl_init() failed" << endl;
    return -1;
  }
  cout << "Successfully call gl_init" << endl;

  size_t buffer_w, buffer_h;
  gl_get_buffer_size(buffer_w, buffer_h);
  cout << "Successfully call gl_get_buffer_size, return "
       << "(" << buffer_w << ", " << buffer_h << ") " << endl;

  vector<unsigned char> pixels;
  size_t t = 0;
  while (gl_window_is_open())
  {
    #ifdef DEBUG_PRINT
    cout << "Loop at t = " << t << endl;
    #endif

    // Poll events. Thia allow time for the window actually appears
    gl_poll_events();

    // update the size
    gl_get_buffer_size(buffer_w, buffer_h);
    pixels.reserve(buffer_w * buffer_h * 4);

    // clear the pixiel buffer
    for (size_t i = 0; i < buffer_h; ++i)
      for (size_t j = 0; j < buffer_w; ++j)
      {
        size_t base = (i * buffer_w + j) * 4;
        pixels[base    ] = 0x0F;
        pixels[base + 1] = 0x77;
        pixels[base + 2] = 0x77;
        pixels[base + 3] = 0xFF;
      }

    // determine the normalized coordinate of the dot
    double phi = t * (3.14f / 180.0f);
    const double r = 0.7;
    double x = (cos(phi) * r + 1.0)/2, y = (sin(phi) * r + 1.0)/2;
    size_t xi = floor(x * buffer_w), yi = floor(y * buffer_h);

    #ifdef DEBUG_PRINT
    cout << "Dot at (" << xi << ", " << yi << ")" << endl;
    #endif

    // fill the dot
    size_t base = (yi * buffer_w + xi) * 4;
    pixels[base    ] = 0xFF;
    pixels[base + 1] = 0xAF;
    pixels[base + 2] = 0x00;
    pixels[base + 3] = 0xFF;

    #ifdef DEBUG_PRINT
    cout << "Dot filled" << endl;
    #endif

    // Draw on the screen.
    // NOTE: this method to get array pointer is discussed by Scott Meyers.
    gl_draw(&pixels[0]);

    // update time
    t += 1;
    t %= 360;
  }

  gl_terminate();

  return 0;
}
