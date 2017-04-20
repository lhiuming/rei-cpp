/**
 * Working along the learnopengl.com tutorials.
 * This is a tutorial code for rendering with texture.
 *
 * Morals:
 *   - Don't forget glEnableVertexAttribArray(i) after setting attributes !!!
 */

#define GLEW_STATIC // for Windows
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

int main()
{
  /////////////////////////////////////////////////////////////////////////////
  // Initialization the GL enviroment.
  // see 'hellow_rectangle.cpp' for more detailed commenting.

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


  /////////////////////////////////////////////////////////////////////////////
  // Set the texture filter method (optional)
  // NOTE: this affects the bound texture object. so the calling below
  // has no actual effect, because we haven't bind any texture object.

  // Option: Set the warpping for coordinate outside the [0, 1]^2 space.
  // NOTE: S for x and T for y.
  // NOTE: Beside mirrored repeat, we can also use:
  //   GL_REPEAT          : (default) just repeats.
  //   GL_CLAMP_TO_EDGE   : repeat the color on the edge/corner.
  //   GL_CLAMP_TO_BORDER : fill the undfined area with a color. this color is
  //     set using glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, c);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

  // Option: Set the method for min/mag filtering
  // NOTE: can use:
  //   GL_NEAREST: nearest filtering
  //   GL_LINEAR:  bi-linear filtering
  // TODO: What happen if we use GL_LINEAR on minifiying ???
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Option: Mipmap method for minify filtering.
  // This should be used with glGenerateMipmaps when creating texture.
  // NOTE: we can use:
  //   GL_NEAREST_MIPMAP_NEAREST: nearest mipmap, use nearest between levels
  //   GL_LINEAR_MIPMAP_NEAREST : nearest mipmap, use bilinear between levels
  //   GL_NEAREST_MIPMAP_LINEAR : linear mipmap
  //   GL_LINEAR_MIPMAP_LINEAR  : linear mipmap
  // NOTE: this is only for _MIN_FILTER; produce error if used on _MAG_FILTER
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);


  /////////////////////////////////////////////////////////////////////////////
  // Set up the texture object

  // Generating a image as source for texture; Usually you might want to load
  // a image file instead of the following.
  size_t image_w = 2, image_h = 2;
  unsigned char image[] = { // just 4 texels
    0xFF, 0x00, 0x00, 0xFF,  // Red
    0x00, 0xFF, 0x00, 0xFF,  // Gren
    0x00, 0x00, 0xFF, 0xFF,  // Blue
    0xAF, 0xA7, 0x07, 0xFF   // Asuka Orange ?
  };

    // Generate a texture Object
    GLuint tex;
    glGenTextures(1, &tex); // allocate 1 texture object id
    glBindTexture(GL_TEXTURE_2D, tex); // set the type and activate it

    // Optional: set a filtering method for the texture object
    // (See section above)

    // Fill the image data after activation
    glTexImage2D(
      GL_TEXTURE_2D, // texture target on the object; will not affect _1D, etc
      0, // create at which mipmap level
      GL_RGBA, // format to store the texture
      image_w, image_h, // result dimension of the texture
      0, // always 0; legacy stuff
      GL_RGBA, GL_UNSIGNED_BYTE, // source format
      image // actual source data
    );

    // Optional: generate the mipmap for the object
    glGenerateMipmap(GL_TEXTURE_2D);

    // Cancel the activation
    glBindTexture(GL_TEXTURE_2D, 0);


  /////////////////////////////////////////////////////////////////////////////
  // Make a rectange (to apply the texture !)
  // See 'hellow_rectangle.cpp' for more detailed commenting.

  // Create a Vertex Array Object (VAO)
  GLuint VAO;
  glGenVertexArrays(1, &VAO); // allocate 1 VAO id
  glBindVertexArray(VAO); // now this VAO is alse "activated"

  // Posiont and Tex-coord buffer
  GLfloat vertices[] = {
    // 2d Position   // Texture Coords
    -0.5f, -0.5f,    0.0f, 0.0f,  // Bottom Left
    -0.5f,  0.5f,    0.0f, 1.0f,  // Top Left
     0.5f,  0.5f,    1.0f, 1.0f,  // Top Right
     0.5f, -0.5f,    1.0f, 0.0f,  // Bottom Right
  };
  GLuint VBO;  // vertex buffer object
  glGenBuffers(1, &VBO); // allocate 1 buffer id
  glBindBuffer(GL_ARRAY_BUFFER, VBO); // binding: specify the type of buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
    GL_STATIC_DRAW); // preferred type of D-RAM in the graphic card

  // Link current buffer to attributes
  // set position attribute at location 0
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
    (GLvoid *)0);
  glEnableVertexAttribArray(0);
  // set tex coord attribute at location 1
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
    (GLvoid *)(2 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);

  // Element buffer
  GLuint indices[] = {
    0, 1, 2,  // starts from 0 !
    0, 2, 3
  };
  GLuint EBO; // Element Buffer Object
  glGenBuffers(1, &EBO); // allocate 1 buffer id
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
    GL_STATIC_DRAW);


  /////////////////////////////////////////////////////////////////////////////
  // Prepare the shader programs that use texture

  // Test returns
  GLint success;
  GLchar infoLog[512];

  // Basically pass through; new feature: it pass the tex coords
  static const char* vertice_shader_text =
  "#version 330 core\n"
  "layout (location = 0) in vec2 position;\n"  // see attribute linking above
  "layout (location = 1) in vec2 texCoord_in;\n"
  "out vec2 texCoord;\n"
  "void main() {\n"
  "  gl_Position = vec4(position, 0.0f,  1.0f);\n"
  "  texCoord = texCoord_in;\n"
  "}\n";
  GLuint vertexShader;
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(
    vertexShader,  // shader object ID
    1,  // number of strings. we have only one
    &vertice_shader_text, // the string source
    nullptr);  // what is this? a hint?
  glCompileShader(vertexShader);
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
    std::cerr << "Error: vertex shader compile failed:\n"
              << infoLog << std::endl;
  }

  // Fragment shader; will look up the texture
  static const char* fragment_shader_text =
  "#version 330 core\n"
  "in vec2 texCoord;\n"
  "out vec4 color;\n"
  "uniform sampler2D tex;\n" // come from the default sampler unit
  "void main() {\n"
  "  //color = texture(tex, texCoord);\n"
  "  color = vec4(texCoord, 0.3, 1.0);\n"
  "}\n";
  GLuint fragmentShader;
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragment_shader_text, nullptr);
  glCompileShader(fragmentShader);
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
    std::cerr << "Error: fragment shader compile failed:\n"
              << infoLog << std::endl;
  }

  // link the compiled shader
  GLuint shaderProgram;
  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);  // attach shader
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram); // linker works ?
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
    std::cerr << "Error: shader linking failed:\n"
              << infoLog << std::endl;
  }

  // Delte the shader object after porgram setup
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  /////////////////////////////////////////////////////////////////////////////
  // Starts the loop
  glClearColor(0.1f, 0.6f, 0.7f, 1.0f); // set the default background color
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents(); // actually we havent set any callback function yet

    // Clear the buffer with color set above with glClearColor(..)
    glClear(GL_COLOR_BUFFER_BIT);

    // Render the two-triangle element
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO); // activate the VAO
    glBindTexture(GL_TEXTURE_2D, tex); // bind the texture before draw
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0); // cancellel the activation

    glfwSwapBuffers(window);
  }

  /////////////////////////////////////////////////////////////////////////////
  // Clean and exit
  glfwTerminate();

  return 0;
}
