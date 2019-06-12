// source of pixels.h
#if OPENGL_ENABLED

#include "pixels.h"

#include <iostream>
#include <map>

#define GLEW_STATIC // for GLEW in Windows
#include <GL/glew.h>     // GLEW, the loader for OpenGL API
#include <GLFW/glfw3.h>  // GLFW, a cross-platform OpenGL utility framework

using namespace std;

namespace CEL {

/////////////////////////////////////////////////////////////////////////////
// Private stuffs

static bool gl_initialized = false;

// Some constant data

static const char* vertex_shader_text =  // vertiex shader source
"#version 330 core\n"
"layout (location = 0) in vec2 position_2d;\n"
"layout (location = 1) in vec2 tex_coord_in;\n"
"out vec2 tex_coord;\n"
"void main() {\n"
"  gl_Position = vec4(position_2d, 0.0, 1.0);\n"  // convert to vec4
"  tex_coord = tex_coord_in;\n"                   // pass-through
"}\n";

static const char* fragment_shader_text =  // fragment shader source
"#version 330 core\n"
"uniform sampler2D tex;\n"
"in vec2 tex_coord;\n"
"out vec4 color;\n"
"void main() {\n"
"  color = texture(tex, tex_coord);\n"
"}\n";

static const GLfloat vertices[] = {
  // positions      // tex-coords
  -1.0f, -1.0f,     0.0f, 0.0f,   // Bottom Left
  -1.0f,  1.0f,     0.0f, 1.0f,   // Top Left
   1.0f,  1.0f,     1.0f, 1.0f,   // Top Right
   1.0f, -1.0f,     1.0f, 0.0f    // Bottom Right
};

static const GLuint indices[] = {
  0, 1, 2,  // top left triangle
  0, 2, 3   // bottom right triangle
};

// a struct to store information for each window
struct Canvas {
  GLuint VAO; // a unique Vertex Array Object ID
  GLuint VBO; // a unique Vertex Buffer Object ID
  GLuint EBO; // a unique Element Buffer Object ID
  GLuint program; // a unique shader program object id
  GLuint texture; // a unique texture object id

  CEL::BufferFunc buffer_call; // a unique buffer size callback
  CEL::ScrollFunc scroll_call; // a unique scroll callback
  CEL::MouseFunc mouse_call; // a unique mouse button callback
  CEL::CursorFunc cursor_call; // a unique cursor position callback

  // constructor
  Canvas() {}
  Canvas(GLuint vao, GLuint vbo, GLuint ebo, GLuint prog, GLuint tex) :
    VAO(vao), VBO(vbo), EBO(ebo), program(prog), texture(tex) {}
};

// a map to look up canvas
static map<GLFWwindow*, Canvas> canvas_table;


// Check is the window id is in the cancas table
int check_canvas(GLFWwindow* window)
{
  if (canvas_table.find(window) == canvas_table.end())
  {
    cerr << "pixels Error: Invalid window id." << endl;
    return -1;
  }
  return 0;
}


// Private procudure to create a new texture object
void create_texture(GLuint* texture_p, int buffer_w, int buffer_h)
{
  // Get a new texture object id
  glGenTextures(1, texture_p);
  glBindTexture(GL_TEXTURE_2D, *texture_p);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, buffer_w, buffer_h);

  // Set the sampling method. Default method is mipmap filering, which
  // we dont need (and will causes rendering error if no mipmap generated)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Clear the binding
  glBindTexture(GL_TEXTURE_2D, 0);
}


// Error callback. Useful when GLFW see something wrong
void error_callback(int error, const char* description)
{
    cerr << "Error: " << description << endl;
}


// Defulat callback for buffer resize. Necessary !
void default_buffer_resize_callback(GLFWwindow* w, int buffer_w, int buffer_h)
{
  // Make the window current (before doing GL things)
  glfwMakeContextCurrent(w);

  // Update the OpenGL viewport
  glViewport(0, 0, buffer_w, buffer_h);

  // Make a new texture object for the resized window
  GLuint* tex_p = &(canvas_table[w].texture);
  glDeleteTextures(1, tex_p); // release the old one
  create_texture(tex_p, buffer_w, buffer_h);  // get a new one with new size
} // end resize_callback


// Buffer resize callback
void uniform_buffer_callback(GLFWwindow* w, int width, int height)
{
  default_buffer_resize_callback(w, width, height); // never forget this !
  canvas_table[w].buffer_call(width, height);
}
// Scroll callback. Used to find and call window-specific callable object
void uniform_scroll_callback(GLFWwindow* w, double x, double y) {
  canvas_table[w].scroll_call(x, y); }
// Mouse button callback. See above
void uniform_mouse_callback(GLFWwindow* w, int button, int action, int mods) {
  canvas_table[w].mouse_call(button, action, mods); }
// Cursor position callback.
void uniform_cursor_callback(GLFWwindow* w, double xpos, double ypos) {
  canvas_table[w].cursor_call(ypos, xpos); /* yes, the order inversed */ }


///////////////////////////////////////////////////////////////////////////////
// Define the interface

// Initialize GLFW and set the default context hints
int gl_init()
{
  // Check if already initlaized
  if (gl_initialized)
    return 0;

  // Initlialize GLFW
  if (!glfwInit())
  {
    cerr << "Error: GLFW intialization failed!" << endl;
    return -1;
  }
  cout << "GLFW initialized. " << endl;

  // Set the default GLFW window hints
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // use OpenGL 4.1 (most
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1); //  update in mac)
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // macOS required
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // modern GL

  // Finished Initialization
  gl_initialized = true;

  return 0;

} // end gl_init

// Create a new window context, and intialize the buffer object, etc.
GLFWwindow* gl_open_window(size_t width, size_t height, const char* s)
{
  GLuint VAO, VBO, EBO, program, texture;
  int buffer_w, buffer_h;

  // Create a GLFW window context and initialize GLEW for it //////////////////

  // Create the GLFW context
  GLFWwindow* window = glfwCreateWindow(width, height, s, nullptr, nullptr);
  if (window == nullptr)
  {
    cerr << "Error: Failed to create a GLFW window" << endl;
    glfwTerminate();
    exit(1);
  }
  glfwMakeContextCurrent(window); // make current before any use
  glfwSetErrorCallback(error_callback); // error call back

  // Initialize GLEW for this context
  glewExperimental = GL_TRUE;  // good with modern core-profile
  if (glewInit() != GLEW_OK)
  {
    cerr << "Error: GLEW initialization failed!" << endl;
    exit(1);
  }
  cout << "GLEW initialized. " << endl;

  // Retrive the buffer dimension of gl context.
  // In HDPI setting, the buffer dimension is lager than the window dimension.
  glfwGetFramebufferSize(window, &buffer_w, &buffer_h);
  glViewport(0, 0, buffer_w, buffer_h);

  // Prepare a shared pass-through style shaders //////////////////////////////

  // Variables for compile error check
  GLint success;
  GLchar infoLog[512];

  // Compile the vertex shader
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertex_shader_text, nullptr);
  glCompileShader(vertexShader);
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) // check if compile pass
  {
    glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
    cerr << "Error: vertex shader compile failed:\n" << infoLog << endl;
  }

  // Compile the fragment shader. Similar above.
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragment_shader_text, nullptr);
  glCompileShader(fragmentShader);
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
    cerr << "Error: fragment shader compile failed:\n" << infoLog << endl;
  }

  // Link the compiled shader into a program;
  program = glCreateProgram();
  glAttachShader(program, vertexShader);  // attach shader
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);
  // test the result as usual
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program, 512, nullptr, infoLog);
    std::cerr << "Error: shader linking failed:\n"
              << infoLog << std::endl;
  }

  // Do not need them after set up the shader program
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  // Set up some vertex buffers ///////////////////////////////////////////////

  // Make a texture object using a private procedure
  create_texture(&texture, buffer_w, buffer_h);

  // Make and activate a Vertex Array Object (VAO)
  // NOTE: following vertex-ralated operation will be stored in this object
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  // Setup the buffer for vertex positions and texture coordinates
  // The rectange should cover the whole window area.
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Link the position and tex-coord buffer to a vertex attribute
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,  // position, at location 0
    4 * sizeof(GLfloat), (GLvoid *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,  // tex-coord, at location 1
    4 * sizeof(GLfloat), (GLvoid *)(2 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);

  // Setup the buffer for vertex index (using Elemtn Buffer Object)
  glGenBuffers(1, &EBO); // allocate 1 buffer id
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
    GL_STATIC_DRAW);

  // Some final settings //////////////////////////////////////////////////////

  // Fill the window with a default color. This also set the default color
  // for followed glClear().
  glClearColor(0.9, 0.4, 0.0, 1.0); // eva blue

  // Set the swaping intercal to 1 (synchronize with monitor refresh rate)
  // May have no effect on some machine. Atleast no effect on mine.
  glfwSwapInterval(1);

  // Allow automatical update the buffer dimension
  glfwSetFramebufferSizeCallback(window, default_buffer_resize_callback);

  // Insert the new window into canvas table
  canvas_table[window] = Canvas(VAO, VBO, EBO, program, texture);

  return window;

} // end gl_create_window


// Retrive the window buffer size. This may be used for making the buffer
// for drawing on canvas.
void gl_get_buffer_size(GLFWwindow* window, size_t &width, size_t &height)
{
  // Retrive the frame buffer size by window id
  int buffer_w, buffer_h;
  glfwGetFramebufferSize(window, &buffer_w, &buffer_h);
  width = (size_t)buffer_w;
  height = (size_t)buffer_h;
}

bool gl_window_should_open(GLFWwindow* window)
{
  return !glfwWindowShouldClose(window);
}

// Set frame buffer resize callback
void gl_set_buffer_callback(GLFWwindow* window, BufferFunc func)
{
  // Store the callable object in table
  if (check_canvas(window)) return;
  canvas_table[window].buffer_call = func;
  // Update glfw for this window
  glfwSetFramebufferSizeCallback(window, uniform_buffer_callback);
}


// Set key-press callback
void gl_set_key_callback(GLFWwindow* window)
{

}

// Set scroll callback
void gl_set_scroll_callback(GLFWwindow* window, ScrollFunc func)
{
  // Store the callable object in table
  if (check_canvas(window)) return;
  canvas_table[window].scroll_call = func;
  // Update glfw for this window
  glfwSetScrollCallback(window, uniform_scroll_callback);
}

// Set mouse button callback
void gl_set_mouse_callback(GLFWwindow* window, MouseFunc func)
{
  // Store the callable object in table
  if (check_canvas(window)) return;
  canvas_table[window].mouse_call = func;
  // Update glfw for this window
  glfwSetMouseButtonCallback(window, uniform_mouse_callback);
}

// Set cursor position update callback
void gl_set_cursor_callback(GLFWwindow* window, CursorFunc func)
{
  // Store the callable object in table
  if (check_canvas(window)) return;
  canvas_table[window].cursor_call = func;
  // Update glfw for this window
  glfwSetCursorPosCallback(window, uniform_cursor_callback);
}

// Get mouse button state
int gl_get_mouse_button(GLFWwindow* window, Button mb)
{
  return glfwGetMouseButton(window, mb);
}

// Get mouse cursor position
void gl_get_cursor_position(GLFWwindow* window, double& i, double& j)
{
  glfwGetCursorPos(window, &j, &i); // Yes, j first, then i.
}

// Poll event
void gl_poll_events()
{
  glfwPollEvents();
}

// Wait event
void gl_wait_events()
{
  glfwWaitEvents();
}

// Draw a pixel buffer
void gl_draw(GLFWwindow* window, char unsigned *pixels, size_t w, size_t h)
{

  // make the window current
  glfwMakeContextCurrent(window); // make current before any use

  // Get the canvas struct
  if (canvas_table.find(window) == canvas_table.end()) {
    cerr << "pixels Error: Invalid window id." << endl;
    return;
  }
  Canvas& canvas = canvas_table[window];

  // Clear the frame buffer
  glClear(GL_COLOR_BUFFER_BIT);

  // Activate related objects
  glUseProgram(canvas.program);
  glBindVertexArray(canvas.VAO); // activate the VAO
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, canvas.EBO);
  glBindTexture(GL_TEXTURE_2D, canvas.texture); // bind texture before draw

  // Load the input buffer into texture
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h,
    GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // Render on the rectangle canvas
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  // Clear the binding
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);

  // Display !
  glfwSwapBuffers(window);
} // end gl_draw


// destroy the window context, and delete the table entry
int gl_close_window(GLFWwindow* window)
{
  // make sure the window is there
  if (canvas_table.find(window) == canvas_table.end())
    return -1;

  Canvas& canvas = canvas_table[window];

  // release the OpenGL resource
  glDeleteVertexArrays(1, &(canvas.VAO));
  glDeleteBuffers(1, &(canvas.VBO));
  glDeleteBuffers(1, &(canvas.EBO));
  glDeleteTextures(1, &(canvas.texture));

  // destroy GLFW window object
  glfwDestroyWindow(window);

  // remove from the table
  canvas_table.erase(window);

  return 0;
}


// Release resources and terminate glfw library
void gl_terminate()
{
  canvas_table.clear();
  glfwTerminate();
}

} // namespace CEL

#endif // OPENGL_ENABLED
