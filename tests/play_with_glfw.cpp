#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdio>

#define WINDOW_W 720
#define WINDOW_H 480

using namespace std;

// error callback function for GLFW
void error_callback(int error, const char* description)
{
    cerr << "Error: " << description << endl;
}

GLuint VAO;
GLuint Buffer; // vertice coordinates buffer id
GLuint Texture; // triangle texture buffer id
GLuint Sampler; // sampler id; should be related to the Texture unit


// this "pass-throuhg" shader comes from the red book
static const char* vertex_shader_text =
"#version 410 core\n"
"layout (location = 0) in vec2 in_position;\n"
"layout (location = 1) in vec2 in_tex_coord;\n"
"out vec2 vs_tex_coord;\n"
"void main()\n"
"{\n"
"    gl_Position = vec4(in_position, 0, 1.0);\n"
"    vs_tex_coord = in_tex_coord;\n"
"}\n";


// this shader comes from the red book
static const char* fragment_shader_text =
"#version 410 core\n"
"in vec2 vs_tex_coord;\n"
"layout (location = 0) out vec3 color;\n"
"uniform sampler2D tex;\n"
"void main()\n"
"{\n"
"    //color = vec4(0, 0.7, 0.6, 0.5);\n"
"    color = texture(tex, vs_tex_coord).rgb;\n"
"    //color = vec4(0, 0.7, 0.6, 0.5).rgb;\n"
"}\n";


void init()
{
  // intialize the window with two triangle and a texture

  // defaut color set to asuka orange
  glClearColor(0.9, 0.4, 0.0, 1.0);

  // create triangle array objects
  glGenVertexArrays(1, &VAO); // retrive a VAO id
  glBindVertexArray(VAO); // allocate memory to the VAO, and focus GL on it

  // specify the vertices
  GLfloat two_triangle_data[] = {
    // vertex positions
    -0.9, -0.9, -0.9, 0.9, 0.9, 0.9, // top-left
    0.9, 0.9, 0.9, -0.9, -0.9, -0.9,  // bottom-right
    // vertex texture coordinates
    0, 0,  0, 1,  1, 1,
    1, 1,  1, 0,  0, 0,
  };

  // create the buffer for vertex data
  glGenBuffers(1, &Buffer);  // retrive a buffer id
  glBindBuffer(GL_ARRAY_BUFFER, Buffer);  // create a buffer object
  glBufferData(GL_ARRAY_BUFFER, sizeof(two_triangle_data),  // allocate and initialize
               two_triangle_data, GL_STATIC_DRAW);          // the buffer

  // set vertex attributes
  // first part is position
  glVertexAttribPointer(0,  // vPosition is with location 0
                        2,  // each vertex take two data (as position)
                        GL_FLOAT, // type of data each vertex takes
                        GL_FALSE, // ask GL not to normalize the data
                        0,  // stride; 0 means read buffer side-by-side/no-skip
                        (GLvoid *)0); // read from the start of the buffer
  glEnableVertexAttribArray(0); // enable the input data of id 0
  // second part is texture coordinates
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0,
    (GLvoid *)(12 * sizeof(float)));
  glEnableVertexAttribArray(1); // enable the input data of id 1

  // create a texture object
  glGenTextures(1, &Texture);  // retrive a texture object id
  glBindTexture(GL_TEXTURE_2D, Texture);  // set the type of the texture

  // allocate memory for the texture
  glTexStorage2D(
    GL_TEXTURE_2D, // is 2D
    0, // no real mipmap
    GL_RGBA8UI, WINDOW_W, WINDOW_H);

}

void update(GLubyte pixels[WINDOW_W * WINDOW_H * 4], GLuint prog)
{
  // Draw pixels to the screen

  // clear buffer before filling
  glClear(GL_COLOR_BUFFER_BIT);

  // re-load the shader program ?
  glUseProgram(prog);

  // update the pixels as texture on the window triangles
  glTexSubImage2D(
    GL_TEXTURE_2D, // as the set up above in init()
    0, // mipmap level
    0, 0, // x and y offset
    WINDOW_W, WINDOW_H, // texture dimension
    GL_RGBA_INTEGER, // texture format; we use 8-bit byte
    GL_UNSIGNED_BYTE, // data type
    pixels  // texture data
   );

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  // draw the triangle, now with new texture
  glBindVertexArray(VAO);  // feed the VAO id; now GL focus on it
  glDrawArrays(GL_TRIANGLES, 0, 6);  // ask GL to draw triangle from the vertices

  glFlush(); // send all commands
}


int main() {

  // initialize GLFW
  if (!glfwInit()) {
      // Initialization failed
      cerr << "GLFW initialization faile!" << endl;
      exit(1);
  }
  glfwSetErrorCallback(error_callback);

  // create a window; Most of them is some what necessary, at leat in macOS
  //glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL

  GLFWwindow* window = glfwCreateWindow(WINDOW_W, WINDOW_H,
     "My Test Window", NULL, NULL);
  if (!window) {
    // Window or OpenGL context creation failed
    cout << "GLFW window creation faile!" << endl;
    exit(1);
  }

  // make and set window context
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  // initialze GLEW; after this, we can use GL core and extension
  GLenum glew_err = glewInit();
  if (glew_err != GLEW_OK) {
    // GLEW intialization failed
    cout << "GLEW Error: " << glewGetErrorString(glew_err) << endl;
    exit(1);
  }

  // omit the setting of callback function for user actions

  // omit enabling setting alpha blending

  cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << endl;
  cout << "Hello, world! with OpenGL and GLEW" << endl;


  // create a gray buffer
  GLubyte pixels[WINDOW_W * WINDOW_H * 4];
  for (size_t i = 0; i < WINDOW_H; ++i)
    for (size_t j = 0; j < WINDOW_W; ++j) {
      pixels[i * WINDOW_H + j    ] = (GLubyte) 128;
      pixels[i * WINDOW_H + j + 1] = (GLubyte) 128;
      pixels[i * WINDOW_H + j + 2] = (GLubyte) 128;
      pixels[i * WINDOW_H + j + 3] = (GLubyte) 128;
    }

  init();

  // load two pass-through shader
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
  glCompileShader(vertex_shader);

  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
  glCompileShader(fragment_shader);

  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  // Keep running when the window is not "closed" by user
  while (!glfwWindowShouldClose(window)){
    // Keep running
    update(pixels, program);
    glfwSwapBuffers(window);
    glfwPollEvents();  // This is necessary !!!
  }

  cout << "Window closed. " << endl;

  // Destroy the window after used.
  glfwDestroyWindow(window);

  // Terminate GLFW before exit
  glfwTerminate();

  return 0;
}
