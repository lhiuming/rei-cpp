// source of gl_viewer.h
#include "gl_viewer.h"

//#include <cmath>

#include <chrono>  // for waiting
#include <thread>  // for waiting

#include "../console.h"

using namespace std;

namespace CEL {

// Static variables
std::map<GLFWwindow*, GLViewer::CallbackMemo> GLViewer::memo_table{};


// Initialize with window size and title
GLViewer::GLViewer(size_t window_w, size_t window_h, string title)
{
  // GLFW library and GLFW context
  init_glfw_context(window_w, window_h, title.c_str());

  // GL env by GLEW
  init_gl_interface();

  // Set all interaction events throught callback function
  register_callbacks();

}

// constructor helpers
void GLViewer::init_glfw_context(int width, int height, const char* s)
{
  // Use a auto-destroy member to init the library
  glfw_init_auto();

  // Create a GLFW window context
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // use OpenGL 4.1 (most
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1); //  update in mac)
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // macOS required
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // modern GL
  this->window = glfwCreateWindow(width, height, s, nullptr, nullptr);
  if (window == nullptr)
  {
    cerr << "Error: Failed to create a GLFW window" << endl;
    throw runtime_error("GLViewer Error: GLFW window initialization FAILED");
  }
  console << "GLFW window created. " << endl;

  glfwMakeContextCurrent(this->window); // activate the context before use
  glfwSetErrorCallback(glfw_error_callback); // simple error call back

  // Set the swaping interval to 1 (synchronize with monitor refresh rate)
  // NOTE: May have no effect on some machine. Atleast no effect on mine.
  glfwSwapInterval(1);

} // end initialize_glfw_context

void GLViewer::init_gl_interface()
{
  // Initialize GLEW in this context
  glewExperimental = GL_TRUE;  // good with modern core-profile
  if (glewInit() != GLEW_OK)
  {
    cerr << "Error: GLEW initialization failed!" << endl;
    glfwTerminate();
    throw runtime_error("GLViewer Error: GLEW initialization FAILED");
  }
  console << "GLEW initialized. " << endl;

  // Retrive the buffer dimension of gl context and setup viewport
  // In HDPI setting, the buffer dimension is lager than the window dimension.
  int buffer_w, buffer_h;
  glfwGetFramebufferSize(window, &buffer_w, &buffer_h);
  glViewport(0, 0, buffer_w, buffer_h);
  console << "GL viewport setup. " << endl;

  // Set the depth buffer
  glEnable(GL_DEPTH_TEST); // yes, we need to enable it manually !
  glDepthRangef(0.0, 1.0); // we looks at +z axis in left-hand coordinate
  glDepthFunc(GL_LESS); // it is default :)

  // Setup a default background color, and fill
  glClearColor(0.9, 0.4, 0.0, 1.0); // EVA blue :P

}

void GLViewer::register_callbacks()
{
  // Some GLFW constant alias
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

  // Create all callback function object (`this` is captured, by value)
  BufferFunc bf = [=](int width, int height) -> void
  {
    this->renderer->set_buffer_size(width, height);
    this->camera->set_ratio((double)width / height);
  };
  ScrollFunc sf = [=](double dx, double dy) -> void
  {
    this->camera->zoom(dy);
  };
  MouseFunc mf = [=](int button, int action, int modkey) -> void
  {
    // TODO repond to mouse-clicks
  };
  CursorFunc cf = [=](double j, double i) -> void
  {
    if (glfwGetMouseButton(this->window, MOUSE_LEFT) == PRESS)
    {
      double dx = this->last_j - j;
      double dy = this->last_i - i;
      this->camera->move(dx * 0.05, 0.0, dy * 0.05); // oppose direction
    }
    // update any way
    this->last_j = j;
    this->last_i = i;
  };

  // Register my own CallbackMemo
  memo_table[window] = CallbackMemo{bf, sf, cf, mf};

  // Bind the uniform pointer to GLFW
  glfwSetFramebufferSizeCallback(window, glfw_bffersize_callback);
  glfwSetScrollCallback(window, glfw_scroll_callback);
  glfwSetCursorPosCallback(window, glfw_cursor_callback);
  glfwSetMouseButtonCallback(window, glfw_mouse_callback);

} // end register_callbacks


// Destructor
GLViewer::~GLViewer()
{ // relase the callback memo and destroy window context
  memo_table.erase(this->window);
  glfwDestroyWindow(this->window);
  console << "A Viewer is closed." << endl;
}


// The update&render loop
void GLViewer::run()
{
  // Make sure the renderer is set corretly
  GLRenderer& gl_renderer = dynamic_cast<GLRenderer&>(*this->renderer);
  gl_renderer.set_gl_context(window);
  gl_renderer.set_scene(scene);
  gl_renderer.set_camera(camera);

  // Start the loop
  while (!glfwWindowShouldClose(window))
  {
    // Activate my context
    glfwMakeContextCurrent(window);

    // Don't forget this (possible alternative: glfwWaitEvents)
    glfwPollEvents();

    // Clear the frame buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // update the scene (it may be dynamics)
    //scene.update();
    //camera->move(0.05, 0.0, 0.0); // fake update
    //camera->set_target(Vec3(0.0, 0.0, 0.0));

    // render the scene on the buffer
    gl_renderer.render();

    // Display !
    glfwSwapBuffers(window);

  } // end while

  console << "user closed window; view stop running" << endl;

} // end run()


// Callback functions; send to GLFW as pointers //

void GLViewer::glfw_error_callback(int error, const char* description)
{ cerr << "Error: " << description << endl; }

void GLViewer::glfw_bffersize_callback(GLFWwindow* window,
  int buffer_w, int buffer_h)
{
  // update viewport; essential action
  glfwMakeContextCurrent(window);  // TODO: should it be restored ?
  glViewport(0, 0, buffer_w, buffer_h);
  // call customizeable actions
  memo_table[window].buffer_callback(buffer_w, buffer_h);
}

void GLViewer::glfw_scroll_callback(GLFWwindow* window, double x, double y)
{ memo_table[window].scroll_callback(x, y); }

void GLViewer::glfw_mouse_callback(GLFWwindow* window,
  int button, int action, int modkey)
{ memo_table[window].mouse_callback(button, action, modkey); }

void GLViewer::glfw_cursor_callback(GLFWwindow* window,
  double dist_to_left, double dist_to_top)
{ memo_table[window].cursor_callback(dist_to_left, dist_to_top); }


// Manage GLFW library using a holder class
class GLFWHolder {
public:
  GLFWHolder() {
    if (!glfwInit()) // actually safe to call multiple times
    {
      cerr << "Error: GLFW intialization failed!" << endl;
      throw runtime_error("GLViewer Error: GLFW intialization FAILED");
    }
    console << "GLFW initialized. " << endl;
  }
  ~GLFWHolder() {
    glfwTerminate();
    console << "GLFW is released" << endl;
  }
};
void GLViewer::glfw_init_auto()
{
  static GLFWHolder glfw_holder;
  return;
}


} // namespace CEL
