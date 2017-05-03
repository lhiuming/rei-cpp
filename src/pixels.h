#ifndef CEL_PIXELS_H
#define CEL_PIXELS_H

#include <cstdlib>  // for size_t
#include <functional> // for std::function

#include <GL/glew.h>    // must include before glfw
#include <GLFW/glfw3.h> // only for defining the Canvas type

/**
 * pixiels.h -- This module provides a wrapper for a naive set of OpenGL API.
 * Bascially, you can use it to create a (or more) OpenGL window, send a pixel
 * buffer to the window context (stored in the Grapihcs Card memory), and
 * then render the pixiels as-it-is on the screen.
 *
 * TODO: implement usefull call-back function setting api, and remove
 *    gl_window_should_open.
 * TODO: make this thread-safe
 */

namespace CEL {

/*
 * Use WindowID to identify different drawable window. This interface supports
 * multiple windows (thanks to GLFW3)
 */
typedef GLFWwindow* WindowID;

/*
 * gl_init -- Initialize the wrapper.
 * Return 0 if successed. It is safe to call this multiple times.
 */
int gl_init();

/*
 * gl_create_window -- Open a window, which is ready for drawing pixels.
 *   width: width of the window
 *   height: height of the window
 *   title: the title of the window
 */
WindowID gl_open_window(std::size_t width, std::size_t height,
  const char* title);

/*
 * gl_get_buffer_size -- Retrive the most up-to-datebuffer size for the given
 * window. Useful for preparing a pixel buffer.
 */
void gl_get_buffer_size(WindowID window,
  std::size_t &width, std::size_t &height);

/*
 * gl_draw -- Render the pixel buffer on the OpenGL windows and display it.
 * Currently the buffer is assumed to be using RGBA8 format (a pixel sample is
 * composed of 4 consecutive 'unsigned char' value).
 *
 * NOTE: The buffer is draw on the screen from the bottom-left corner, so
 * screen coordinate (0, 0) should be map to the first slot of the buffer.
 * For example, you should map (x, y) to the buffer index doing like:
 *     pixels[y * width + x] = a_pixel_color_data
 */
void gl_draw(WindowID window, char unsigned *pixels,
  std::size_t buffer_w, std::size_t buffer_h);

/*
 * Some constants for writing callback function.
 */
enum Button : int {
  MOUSE_LEFT = GLFW_MOUSE_BUTTON_LEFT,
  MOUSE_MIDDLE = GLFW_MOUSE_BUTTON_MIDDLE,
  MOUSE_RIGHT = GLFW_MOUSE_BUTTON_RIGHT };
enum Action : int {
  PRESS = GLFW_PRESS,
  RELEASE = GLFW_RELEASE };
enum ModKey : int {
  ALT = GLFW_MOD_ALT,
  CTL = GLFW_MOD_CONTROL,
  SHIFT = GLFW_MOD_SHIFT,
  SUPER = GLFW_MOD_SUPER };

/*
 * gl_set_buffer_resize_callback -- Set the callback function when the
 * framebuffer is resized by the user or system.
 */
using BufferFunc = std::function<void (int w, int h)>;
void gl_set_buffer_callback(WindowID window, BufferFunc func);

/*
 * gl_set_key_callback -- Set the callback function when keys are pressed
 * by the user. Useful into making interaction.
 * TODO: implement me
 */
void gl_set_key_callback(WindowID window);

/*
 * gl_set_scroll_callback -- Similar to above. Unique for a window.
 */
using ScrollFunc = std::function<void (double dx, double dy)>;
void gl_set_scroll_callback(WindowID window, ScrollFunc func);

/*
 * gl_set_mouse_pos_callback -- TODO: implement me
 */
void gl_set_mouse_pos_callback();

/*
 * gl_set_mouse_callback -- Similar to above. Unique for a window.
 * Use Botton, Action and ModKey value in the callable object.
 * TODO: test me
 */
using MouseFunc = std::function<void (int button, int action, int modkey)>;
void gl_set_mouse_callback(WindowID window, MouseFunc func);

/*
 * gl_set_cursor_callback -- Similar to above. Unique for a window.
 */
using CursorFunc = std::function<void (double i, double j)>;
void gl_set_cursor_callback(WindowID window, CursorFunc);

/*
 * gl_get_mouse_button -- Check the press/release state.
 * Use (MOUSE_) Button value in mouse_button argument.
 * Use Action to compare with the returned value.
 */
int gl_get_mouse_button(WindowID window, Button mouse_button);

/*
 * gl_get_cursor_position -- Return the position of user cursor. Useful
 * in writting the mouse button callback function.
 *
 * The result (i, j) is screen coordinate relative to the upper-left corner.
 * Think that i is "row number", j is "column number".
 */
void gl_get_cursor_position(WindowID window, double& i, double& j);

/*
 * gl_poll_events -- Poll the events and call the callback functions.
 */
void gl_poll_events();

/*
 * gl_wait_events -- Wait for an event to return. Useful for updating static
 * scene.
 */
void gl_wait_events();

/*
 * gl_window_is_open -- Return true if the window is set to be closed (by
 * checking the state of an internal closing flag)
 */
bool gl_window_should_open(WindowID window);

/*
 * gl_destroy_window -- Release the resource of the window object.
 * Return -1 if the window is not valid (or already closed)
 */
int gl_close_window(WindowID window);

/*
 * gl_terminate -- Finish using the pixel library. Will release resouces.
 */
void gl_terminate();

} // namespace CEL

#endif // CEL_PIXELS_H
