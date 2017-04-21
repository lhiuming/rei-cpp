/**
 * pixiels.h -- This module provides a wrapper for a naive set of OpenGL API.
 * Bascially, you can use it to create a OpenGL window, send a pixel
 * buffer to the window context (stored in the Grapihcs Card memory), and
 * then render the pixiels as-it-is on the screen.
 *
 * NOTE: this wrapper currently only support one window.
 */
#ifndef CEL_PIXELS_H
#define CEL_PIXELS_H

#include <cstdlib>

namespace CEL {

/*
 * gl_init -- Initialize the OpenGL enviroment and a window context. Return
 * -1 if initialization fails.
 *   width: width of the window
 *   height: height of the window
 */
int gl_init(std::size_t width, std::size_t height, const char* title);

/*
 * gl_get_buffer_size -- Retrive the buffer size for drawing pixels.
 */
void gl_get_buffer_size(std::size_t &width, std::size_t &height);

/*
 * gl_draw -- Render the pixel buffer on the OpenGL windows and display.
 * We assume the dimension of the buffer agrees with gl_get_buffer_size.
 * NOTE: I think the user should use some other interface ...
 */
void gl_draw(char unsigned *pixels);

/*
 * gl_set_key_callback -- Set the callback function when keys are pressed
 * by the user. Useful into making interaction.
 * TODO: implement me
 */
void gl_set_key_callback();

/*
 * gl_poll_event -- Poll the events and call the callback functions.
 * TODO: implement me. But implement gl_set_key_callback first
 */
void gl_poll_events();

/*
 * gl_window_is_open -- Return true if the window is not closed.
 */
bool gl_window_is_open();

/*
 * gl_terminate -- Finishe using the GL window.
 */
void gl_terminate();

} // namespace CEL

#endif // CEL_PIXELS_H
