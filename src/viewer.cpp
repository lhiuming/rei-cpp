// source of viewer.h
#include "viewer.h"

#include <vector>
#include <chrono>  // for waiting
#include <thread>  // for waiting
#include <iostream> // for debug

using namespace std;

namespace CEL {

// Static variable initialization
int Viewer::view_count = 0;

// A Simple constructor
Viewer::Viewer(size_t window_w, size_t window_h, string title)
{
  // open a window
  gl_init();
  this->window = gl_open_window(window_w, window_h, title.c_str());
  // initialize an internal buffer
  gl_get_buffer_size(this->window, this->buffer_w, this->buffer_h);
  pixels.reserve(buffer_w * buffer_h * 4);

  // increase the global count
  ++view_count;
}

// Deconstructor
Viewer::~Viewer()
{
  gl_close_window(window);  // close the window if not already closed
  --view_count;
  if (view_count == 0) gl_terminate();
  cout << "A Viewer is closed." << endl;
}

// Opeartions

// Set a renderer
void Viewer::set_renderer(Renderer* renderer)
{
  this->renderer = renderer;
}

// Set a scene (will be passed to renderer)
void Viewer::set_scene(Scene* scene)
{
  this->scene = scene;
}

// The update&render loop
void Viewer::run()
{
  auto start = chrono::system_clock::now();
  chrono::duration<double> time_limit(3.14);
  cout << "This viewer is alive for 3.14 secs." << endl;

  // start the loop
  while (gl_window_should_open(window))
  {
    // debug check
    if (chrono::system_clock::now() - time_limit > start )
      break;

    // Don't forget this
    gl_poll_events();

    // update the scene (it may be dynamics)
    //scene.update();

    // Get the newest buffer size
    update_buffer_size();

    // render the scene on the buffer
    //renderer.render(scene, &pixels[0], buffer_w, buffer_h);

    // just wait
    sleep_alittle();

    // draw on the window
    gl_draw(window, &pixels[0], buffer_w, buffer_h);
  } // end while

} // end run()


// Private helpers

void Viewer::update_buffer_size()
{
  gl_get_buffer_size(this->window, this->buffer_w, this->buffer_h);
  pixels.reserve(buffer_w * buffer_h * 4);

}

void Viewer::sleep_alittle()
{
  using namespace this_thread; // sleep_for, sleep_until
  using namespace chrono; // nanoseconds, system_clock, seconds

  //sleep_for(nanoseconds(10));
  sleep_until(system_clock::now() + milliseconds(40));
}

} // namespace CEL
