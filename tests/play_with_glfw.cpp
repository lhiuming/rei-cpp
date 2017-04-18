#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdio>

using namespace std;

// Error callback function for GLFW
void error_callback(int error, const char* description)
{
    cerr << "Error: " << description << endl;
}


// Main function
int main() {

  // Initilaize GLFW
  if (!glfwInit()) {
      // Initialization failed
      cerr << "GLFW initialization faile!" << endl;
  }

  // Setting GLFW
  glfwSetErrorCallback(error_callback);
  //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  // Create first window !
  GLFWwindow* window = glfwCreateWindow(640, 480, "My Title", NULL, NULL);
  if (!window) {
    // Window or OpenGL context creation failed
    cout << "GLFW window creation faile!" << endl;
  }

  // Create content on the window
  glfwMakeContextCurrent(window);
  GLenum glew_err = glewInit();
  if (glew_err != GLEW_OK) {
    // GLEW intialization failed
    fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(glew_err));
  }
  cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << endl;

  cout << "Hello, world! with OpenGL and GLEW" << endl;

  // Destroy the window after used.
  glfwDestroyWindow(window);

  // Terminate GLFW before exit
  glfwTerminate();

  return 0;
}
