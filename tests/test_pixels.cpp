// Test the OpenGL API wrapper (pixels.h)
// Create a window and draw a spining dot

#include <cassert>
#include <cmath>

#include <iostream>
#include <vector>

#include <opengl/pixels.h>

//#define DEBUG_PRINT

using namespace std;
using namespace CEL;

int main() {
  if (gl_init()) {
    cerr << "Call gl_init() failed" << endl;
    return -1;
  }
  cout << "Successfully call gl_init" << endl;

  // this should fail
  // WindowID window0 = gl_create_window(0, 0, "test_pixels .cpp");

  WindowID window1 = gl_open_window(720, 480, "test_pixels 1.cpp");
  WindowID window2 = gl_open_window(480, 720, "test_pixels 2.cpp");

  size_t buffer_w, buffer_h;
  gl_get_buffer_size(window1, buffer_w, buffer_h);
  cout << "Successfully call gl_get_buffer_size for window 1, return "
       << "(" << buffer_w << ", " << buffer_h << ") " << endl;

  // Initialize the buffer
  vector<unsigned char> pixels;
  pixels.reserve(buffer_w * buffer_h * 4);
  for (size_t i = 0; i < buffer_h; ++i)
    for (size_t j = 0; j < buffer_w; ++j) {
      size_t base = (i * buffer_w + j) * 4;
      pixels[base] = 0x0F;
      pixels[base + 1] = 0x77;
      pixels[base + 2] = 0x77;
      pixels[base + 3] = 0xFF;
    }

  size_t t = 0;
  while (gl_window_should_open(window1) && gl_window_should_open(window2)) {
#ifdef DEBUG_PRINT
    cout << "Loop at t = " << t << endl;
#endif

    WindowID window = window2;
    if ((t % 2) == 1) window = window1;

    // Poll events. Thia allow time for the window actually appears
    gl_poll_events();

    // Update the size
    gl_get_buffer_size(window, buffer_w, buffer_h);
    pixels.reserve(buffer_w * buffer_h * 4);

    // determine the normalized coordinate of the dot
    double phi = t * (3.14f / 180.0f);
    const double r = 0.7;
    double x = (cos(phi) * r + 1.0) / 2, y = (sin(phi) * r + 1.0) / 2;
    size_t xi = floor(x * buffer_w), yi = floor(y * buffer_h);

#ifdef DEBUG_PRINT
    cout << "Dot at (" << xi << ", " << yi << ")" << endl;
#endif

    // Fill the dot
    size_t base = (yi * buffer_w + xi) * 4;
    pixels[base] = 0xFF;
    pixels[base + 1] = 0xAF;
    pixels[base + 2] = 0x00;
    pixels[base + 3] = 0xFF;

#ifdef DEBUG_PRINT
    cout << "Dot filled" << endl;
#endif

    // Draw on the screen.
    // NOTE: this method to get array pointer is discussed by Scott Meyers.
    gl_draw(window, &pixels[0], buffer_w, buffer_h);

    // update time
    t += 1;
    t %= 360;
  }

  assert(gl_close_window(window1) == 0);
  cout << "Close window 1 " << endl;

  assert(gl_close_window(window2) == 0);
  cout << "Close window 2 " << endl;

  // test the resutn
  assert(gl_close_window(window1) == -1);
  cout << "Successfully detect a second close on window 1" << endl;

  gl_terminate();

  return 0;
}
