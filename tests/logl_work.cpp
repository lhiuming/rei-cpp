/**
 * Working along the learnopengl.com tutorials.
 *
 * Morals:
 *   1. spell the library constant correctly !!!! You don't want to mis-spell
 *      GLFW_OPENGL_PROFILE as GLFW_OPENGL_CORE_PROFILE and spend a whole
 *      hour to find the typo bug !!!! ABSOLUTELY NO PLUGIN CAN HELP YOU !!!
 */

#define GLEW_STATIC // need in Windows
#include <GL/glew.h>  // do this before including glfw
#include <GLFW/glfw3.h>

#include <iostream>

// Error callback. Useful when GLFW see something wrong.
void error_callback(int error, const char* description)
{
    std::cerr << "Error from glfw: " << description << std::endl;
}

// Key-press callback for ESC
void key_callback(GLFWwindow* window, int key, int scancode, int action,
  int mode)
{
  // when user presses ESC, set WindowShouldClose (which will close the window)
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
}

int main()
{

  ////////////////////////////////////////////////
  // initialize and configure the a openGL window
  // see
  //    http://www.glfw.org/docs/latest/window.html#window_hints
  // for more details
  if (!glfwInit())
  {
    std::cerr << "GLFW intialization failed!" << std::endl;
    return -1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // use OpenGL 3.3
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // macOS required
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // modern
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);  // user cannot resize window !

  /////////////////////////////////////////
  // Make a window context
  glfwSetErrorCallback(error_callback);
  GLFWwindow* window = glfwCreateWindow(720, 480, "LearnOpenGL",
    nullptr, nullptr);
  if (window == nullptr)
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  ///////////////////////////////////////////
  // Initialize GLFW before making gl calls
  glewExperimental = GL_TRUE;  // good with modern setting / core-profile
  if (glewInit() != GLEW_OK)
  {
    std::cout << "Failed to initialize GLEW" << std::endl;
    return -1;
  }

  //////////////////////////
  // Seting the dimensino of gl context
  int width, height;
  glfwGetFramebufferSize(window, &width, &height); // retrive from glfw
  glViewport(0, 0, width, height); // safe even in HDPI mode


  /////////////////////////////
  // Register some key event
  glfwSetKeyCallback(window, key_callback);

  /////////////////////////////
  // Starts the game loop
  glClearColor(0.1f, 0.6f, 0.7f, 1.0f); // set the default background color
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents(); // actually we havent set any callback function yet

    // Some rendering works
    glClear(GL_COLOR_BUFFER_BIT);

    glfwSwapBuffers(window);
  }

  /////////////////////////
  // Clean and exit
  glfwTerminate();

  return 0;
}
