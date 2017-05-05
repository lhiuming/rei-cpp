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
{ this->renderer = renderer; }

// Set a scene (will be passed to renderer)
void Viewer::set_scene(Scene* scene)
{ this->scene = scene; }

// Set a camera (will be passed to renderer)
void Viewer::set_camera(Camera* camera)
{ this->camera = camera; }


// The update&render loop
void Viewer::run()
{
  // make sure the renderer is set corretly
  size_t w, h;
  gl_get_buffer_size(window, w, h);
  renderer->set_buffer_size(w, h);
  renderer->set_scene(scene);
  renderer->set_camera(camera);
  renderer->set_draw_func(make_buffer_draw());

  // update callback function
  gl_set_buffer_callback(window, make_buffer_callback());
  gl_set_scroll_callback(window, make_scroll_callback());
  gl_set_cursor_callback(window, make_cursor_callback());

  // start the loop
  while (gl_window_should_open(window))
  {
    // Don't forget this
    gl_poll_events();

    // update the scene (it may be dynamics)
    //scene.update();

    // render the scene on the buffer
    renderer->render();

  } // end while

  cout << "user closed window; view stop running" << endl;

} // end run()


// Make a callable draw function for soft renderer
DrawFunc Viewer::make_buffer_draw() const
{
  return [=](unsigned char* b, size_t w, size_t h) -> void
         { gl_draw(this->window, b, w, h); };
}

// Make a buffer resize callback. See pixels.h
BufferFunc Viewer::make_buffer_callback() const
{
  return [=](int width, int height) -> void
         { this->renderer->set_buffer_size(width, height);
           this->camera->set_ratio((double)width / height); };
}

// Make scroll callback. See pixels.h
ScrollFunc Viewer::make_scroll_callback() const
{
  return [=](double dx, double dy) -> void
         { this->camera->zoom(dy); };
}

// Make cursor position callback. See pixels.h
CursorFunc Viewer::make_cursor_callback() const
{
  return [=](double i, double j) -> void
         {
           if (gl_get_mouse_button(this->window, MOUSE_LEFT) == PRESS) {
             double dx = this->last_j - j;
             this->camera->move(dx * 0.05, 0.0, 0.0); // oppose direction
           } // end if
           // update any way
           this->last_j = j;
           this->last_i = j;
         };
}

} // namespace CEL
