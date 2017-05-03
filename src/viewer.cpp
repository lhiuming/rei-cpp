// source of viewer.h
#include "viewer.h"

#include <cstddef>
#include <cmath>

#include <vector>
#include <chrono>  // for waiting
#include <thread>  // for waiting
#include <iostream> // for debug

using namespace std;

namespace CEL {

// Static variable initialization
int Viewer::view_count = 0;

// Initialize with window size and title
Viewer::Viewer(size_t window_w, size_t window_h, string title)
{
  // open a window
  gl_init();
  this->window = gl_open_window(window_w, window_h, title.c_str());

  // increase the global count
  ++view_count;
}

// Deconstructor; a little bit work
Viewer::~Viewer()
{
  gl_close_window(window);  // close the window anyway
  if (--view_count == 0) gl_terminate();
  cout << "A Viewer is closed." << endl;
}

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

// Set a camera (will be passed to renderer)
void Viewer::set_camera(Camera* camera)
{
  this->camera = camera;
}

// The update&render loop
void Viewer::run()
{
  // make sure the renderer is set corretly
  renderer->set_scene(scene);
  renderer->set_camera(camera);
  renderer->set_draw_func(make_buffer_draw());

  // update callback function
  gl_set_scroll_callback(window, make_scroll_callback());
  gl_set_cursor_callback(window, make_cursor_callback());

  // start the loop
  while (gl_window_should_open(window))
  {
    // Don't forget this
    gl_poll_events();

    // Get the newest buffer size
    // TODO: embed in gl callback
    update_buffer_size();

    // update the scene (it may be dynamics)
    //scene.update();

    // render the scene on the buffer
    renderer->render();

    // just wait
    sleep_alittle();
  } // end while

  cout << "user closed window; view stop running" << endl;

} // end run()


// Get the buffer size of the window
void Viewer::update_buffer_size() const
{
  size_t width, height;
  gl_get_buffer_size(this->window, width, height);
  renderer->set_buffer_size(width, height);
}

// Make a callable draw function for soft renderer
DrawFunc Viewer::make_buffer_draw() const
{ return [=](unsigned char* b, size_t w, size_t h) -> void
         { gl_draw(this->window, b, w, h); };
}

// Make scroll callback. See pixels.h
ScrollFunc Viewer::make_scroll_callback() const
{ return [=](double dx, double dy) -> void
         { this->camera->zoom(dy); };
}

// Make cursor position callback. See pixels.h
CursorFunc Viewer::make_cursor_callback()
{ return [=](double i, double j) -> void
         {
           if (gl_get_mouse_button(this->window, MOUSE_LEFT) == PRESS) {
             double dx = j - this->last_j;
             this->camera->move(- dx / 50, 0.0, 0.0); // oppose direction
             cout << "move dx = " << dx << endl;
           } else { // must be RELEASR
             this->last_j = j;
           } // end if
         };
}

// Pause the loop (e.g. to maintain a low refresh rate)
void Viewer::sleep_alittle() const
{
  using namespace this_thread; // sleep_for, sleep_until
  using namespace chrono; // nanoseconds, system_clock, seconds

  //sleep_for(nanoseconds(10));
  sleep_until(system_clock::now() + milliseconds(40));
}

} // namespace CEL
