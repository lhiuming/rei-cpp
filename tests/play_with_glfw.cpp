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

void update(size_t width, size_t height, unsigned char *pixels)
{
  glClear(GL_COLOR_BUFFER_BIT);

  // copy pixels to the screen
//glPushAttrib( GL_VIEWPORT_BIT );
//glViewport(0, 0, width, height);

//glMatrixMode( GL_PROJECTION );
//glPushMatrix();
//glLoadIdentity();
//glOrtho( 0, width, 0, height, 0, 0 );

//glMatrixMode( GL_MODELVIEW );
//glPushMatrix();
//glLoadIdentity();
//glTranslatef( -1, 1, 0 );

//glRasterPos2f(0, 0);
//glPixelZoom( 1.0, -1.0 );
glDrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
//glPixelZoom( 1.0, 1.0 );

//glPopAttrib();
//glMatrixMode( GL_PROJECTION ); glPopMatrix();
//glMatrixMode( GL_MODELVIEW  ); glPopMatrix();

}

int main() {

  // initialize GLFW
  if (!glfwInit()) {
      // Initialization failed
      cerr << "GLFW initialization faile!" << endl;
      exit(1);
  }
  glfwSetErrorCallback(error_callback);

  // create a window
  GLFWwindow* window = glfwCreateWindow(WINDOW_W, WINDOW_H, "My Test Window", NULL, NULL);
  if (!window) {
    // Window or OpenGL context creation failed
    cout << "GLFW window creation faile!" << endl;
    exit(1);
  }

  // make and set window context
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  // initialze GLEW; after this, we can use GL core and extension functionality
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
  unsigned char pixels[WINDOW_W * WINDOW_H * 4];
  for (size_t i = 0; i < WINDOW_W; ++i)
    for (size_t j = 0; j < WINDOW_H; ++j) {
      ; // just show the random data from memory
    }

  // Keep running when the window is not "closed" by user
  while (!glfwWindowShouldClose(window)){
    // Keep running
    update(720, 480, pixels);

    glfwSwapBuffers(window);
  }

  cout << "Window closed. " << endl;

  // Destroy the window after used.
  glfwDestroyWindow(window);

  // Terminate GLFW before exit
  glfwTerminate();

  return 0;
}
