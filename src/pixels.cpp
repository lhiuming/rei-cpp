// source of pixels.h
#include "pixels.h"

#include <iostream>

#define GLEW_STATIC // for GLEW in Windows
#include <GL/glew.h>     // GLEW, the loader for OpenGL API
#include <GLFW/glfw3.h>  // GLFW, a cross-platform OpenGL utility framework

using namespace std;

/////////////////////////////////////////////////////////////////////////////
// Private stuffs

static int buffer_w, buffer_h; // current buffer dimension
static GLFWwindow* window; // current GLFW window

static GLuint VAO; // the only Vertex Array Object ID
static GLuint VBO; // the only Vertex Buffer Object ID
static GLuint EBO; // the only Element Buffer Object ID
static GLuint texture; // the only texture object
static GLuint program; // the only shader program (object id)

// Error callback. Useful when GLFW see something wrong
void error_callback(int error, const char* description)
{
    cerr << "Error: " << description << endl;
}

// Window resize call back. Just update things when the frame buffer is
// changed by user or system.
void resize_callback(GLFWwindow* resized_window, int width, int height)
{
  // We should have only one windows.
  if (window != resized_window) {
    cerr << "Error: wrong window object in resize_callback" << endl;
    exit(1);
  }

  // Update private variables
  buffer_w = width;
  buffer_h = height;

  // update the OpenGL viewport
  glViewport(0, 0, width, height);

  // Update the texture size
  // TODO: implement me

} // end resize_callback


/////////////////////////////////////////////////////////////////////////////
// Define the interface

namespace CEL {

// Initialize the GL window and prepare the buffer for input pixels
int gl_init(size_t height, size_t width, const char* title)
{

  // Initialization of the GL enviroment ////////////////

  // Initlialize glfw
  if (!glfwInit())
  {
    cerr << "Error: GLFW intialization failed!" << endl;
    return -1;
  }

  // Make a GLFW window context
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // use OpenGL 3.3
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // macOS required
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // modern GL
  glfwSetErrorCallback(error_callback);
  window = glfwCreateWindow(height, width, "LearnOpenGL",
    nullptr, nullptr);
  if (window == nullptr)
  {
    cerr << "Error: Failed to create a GLFW window" << endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  // Initialize GLEW before making any gl calls
  glewExperimental = GL_TRUE;  // good with modern core-profile
  if (glewInit() != GLEW_OK)
  {
    cerr << "Error: GLEW initialization failed!" << endl;
    return -1;
  }

  // Retrive the buffer dimension of gl context.
  // In HDPI setting, the buffer dimension is lager than the window dimension.
  glfwGetFramebufferSize(window, &buffer_w, &buffer_h);
  glViewport(0, 0, buffer_w, buffer_h);

  // Create the canvas for drawing pixels ////////////////

  // Make a texture object
  glGenTextures(1, &texture);  // retrive a texture object id
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, buffer_w, buffer_h);

  // Set the sampling method. Default method is mipmap filering, which
  // we dont need (and will causes rendering error if no mipmap generated)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Make and activate a Vertex Array Object (VAO)
  // NOTE: following vertex-ralated operation will be stored in this object
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  // Setup the buffer for vertex positions and texture coordinates
  // The rectange should cover the whole window area.
  // TODO: currently is half of the windows; make it full
  GLfloat vertices[] = {
    // positions      // tex-coords
    -0.5f, -0.5f,     0.0f, 0.0f,   // Bottom Left
     0.5f, -0.5f,     0.0f, 1.0f,   // Top Left
     0.5f,  0.5f,     1.0f, 1.0f,   // Top Right
    -0.5f,  0.5f,     1.0f, 0.0f    // Bottom Right
  };
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
  GLuint indices[] = {
    0, 1, 2,  // top left triangle
    0, 2, 3   // bottom right triangle
  };
  glGenBuffers(1, &EBO); // allocate 1 buffer id
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
    GL_STATIC_DRAW);

  // Prepare the pass-through style shader
  GLint success;  // for compile error check
  GLchar infoLog[512];
  const char* vertex_shader_text =  // vertiex shader source
  "#version 330 core\n"
  "layout (location = 0) in vec2 position_2d;\n"
  "layout (location = 1) in vec2 tex_coord_in;\n"
  "out vec2 tex_coord;\n"
  "void main() {\n"
  "  gl_Position = vec4(position_2d, 0.0, 1.0);\n"  // convert to vec4
  "  tex_coord = tex_coord_in;\n"                   // pass-through
  "}\n";
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertex_shader_text, nullptr);
  glCompileShader(vertexShader);
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) // check if compile pass
  {
    glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
    cerr << "Error: vertex shader compile failed:\n" << infoLog << endl;
  }
  const char* fragment_shader_text =  // fragment shader source
  "#version 330 core\n"
  "uniform sampler2D tex;\n"
  "in vec2 tex_coord;\n"
  "out vec4 color;\n"
  "void main() {\n"
  "  color = texture(tex, tex_coord);\n"
  "}\n";
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
  glDeleteShader(vertexShader);  // dont need them after set up the program
  glDeleteShader(fragmentShader);

  // Fill the window with a default color. This also set the default color
  // for followed glClear().
  glClearColor(0.9, 0.4, 0.0, 1.0); // eva blue

  return 0;

} // end gl_init

// Retrive the window buffer size. This may be used for making the buffer
// for drawing on canvas.
void gl_get_buffer_size(size_t &width, size_t &height)
{
  // TODO: add range check of dynamic casting between size_t and int
  width = (size_t)buffer_w;
  height = (size_t)buffer_h;
}

inline bool gl_window_is_open()
{
  return !glfwWindowShouldClose(window);
}

// Draw a pixel buffer
void gl_draw(char unsigned *pixels)
{
  // Clear the frame buffer
  glClear(GL_COLOR_BUFFER_BIT);

  // Activate related objects
  glUseProgram(program);
  glBindVertexArray(VAO); // activate the VAO
  glBindTexture(GL_TEXTURE_2D, texture); // bind the texture before draw

  // Load the input buffer into texture
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer_w, buffer_h,
    GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // Render on the rectangle canvas
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  // Clear the binding
  glBindVertexArray(0); // cancellel the activation

  // Display !
  glfwSwapBuffers(window);
} // end gl_draw

// Terminate glfw
void gl_terminate()
{
  glfwTerminate();
}

} // namespace CEL
