/**
 * Working along the learnopengl.com tutorials.
 * This is a tutorial code for rendering with texture.
 */

#include GLEW_STATIC // for Windows
#include <GL/glew.h>
#include <GLFW/glfw3.h>

int main()
{
  /////////////////////////////////////////////////////////////////////////////
  // Initialization the GL enviroment.
  // see 'hellow_rectangle.cpp' for more detailed comments.

  // Initlialize glfw
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

  // Make a window context
  //glfwSetErrorCallback(error_callback); // no error call back
  GLFWwindow* window = glfwCreateWindow(720, 480, "LearnOpenGL",
    nullptr, nullptr);
  if (window == nullptr)
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  // Initialize GLFW before making gl calls
  glewExperimental = GL_TRUE;  // good with modern setting / core-profile
  if (glewInit() != GLEW_OK)
  {
    std::cout << "Failed to initialize GLEW" << std::endl;
    return -1;
  }

  // Seting the dimension of gl context
  int width, height;
  glfwGetFramebufferSize(window, &width, &height); // retrive from glfw
  glViewport(0, 0, width, height); // safe even in HDPI mode


  return 0;
}
