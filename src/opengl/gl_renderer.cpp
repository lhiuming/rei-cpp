// source of renderer.h (in opengl/)
#include "gl_renderer.h"

#include <cstring>
#include <iostream>
#include <typeinfo>

using namespace std;

static const char* gl_vertex_shader_text =  // vertiex shader source
"#version 410 core\n"
"layout (location = 0) in vec4 vPosition;\n"
"layout (location = 1) in vec4 vColor;\n"
"out vec4 color;\n"
"void main() {\n"
"  gl_Position = vPosition;\n"
"  color = vColor;\n"
"}\n";

static const char* gl_fragment_shader_text =  // fragment shader source
"#version 410 core\n"
"in vec4 color;\n"
"out vec4 fcolor;\n"
"void main() {\n"
"  fcolor = color;\n"
"}\n";

namespace CEL {

// Default constructor
GLRenderer::GLRenderer() : Renderer()
{
  // Prepare a shared pass-through style shaders //////////////////////////////

  // Variables for compile error check
  GLint success;
  GLchar infoLog[512];
cout << "spot 0" << endl;
  // Compile the vertex shader
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER); cout << "spot 1" << endl;
  glShaderSource(vertexShader, 1, &gl_vertex_shader_text, nullptr);cout << "spot 2" << endl;
  glCompileShader(vertexShader);cout << "spot 1" << endl;
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);cout << "spot 3" << endl;
  if (!success) // check if compile pass
  {
    cout << "trying to report failure" << endl;
    glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
    cerr << "Error: vertex shader compile failed:\n" << infoLog << endl;
  }

  // Compile the fragment shader. Similar above.
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &gl_fragment_shader_text, nullptr);
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

}

// Set buffer size
void GLRenderer::set_buffer_size(BufferSize width, BufferSize height)
{
  // TODO
  // use GL api
}


// Render request
void GLRenderer::render()
{
  // TODO: devide this into stages like a hardware pipeline
  // 1. transform the model into camera space (need some memory)
  //   - may be every data should be converted to "render vertex" now,
  //     or any data that is good for renderering.
  //   - may be some shared accelerating helpers, like a BVH tree for global
  //     illuminating. (but how many stuffs are shared among models, really?)
  // (different models should be passed to different handling function now)
  // 2. do vertex shading on the scene (result stored in vertex)
  // 3. project by Mat4 (perspective of orthographic)
  // 4. clipping (or naive clipping) againt the normalized unit cube
  // 5. screen mapping ( (0, width) x (0, height) )
  // 6. rasterrize by primitives (interpolating, color merging into pixels)
  //   - maybe support some fragment shader things.
  //   - see RTR notes for detials

  // Make sure the scene and camera is set
  if (scene == nullptr) {
    cerr << "SoftRenderer Error: no scene! " << endl;
    return;
  }
  if (camera == nullptr) {
    cerr << "SoftRenderer Error: no camera! " << endl;
    return;
  }

  // make the window current (activate)
  glfwMakeContextCurrent(this->window);

  // Set the rendering methos
  glEnable(GL_DEPTH_TEST); // yes, we need to enable it manually !
  glDepthRangef(0.0, 1.0); // we looks at +z axis in left-hand coordinate 
  glDepthFunc(GL_LESS); // it is default :)

  // Clear the frame buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Activate shader programs
  glUseProgram(this->program);

  // Fetch and render all models (set vao, ebo, etc and draw)
  for (const auto& mi : scene->get_models() )
  {
    const Model& model = *(mi.pmodel);
    const Mat4& trans = mi.transform;

    // choose a rendering procedure
    const auto& model_type = typeid(model);
    if (model_type == typeid(Mesh)) {
      rasterize_mesh(dynamic_cast<const Mesh&>(model), trans);
    } else {
      cerr << "Error: Unkown model type: "<< model_type.name() << endl;
    }
  } // end for

  // Display !
  glfwSwapBuffers(window);
}


void GLRenderer::rasterize_mesh(const Mesh& mesh, const Mat4& trans)
{
  // 1. create vertex array object
  GLuint vao;
  glGenVertexArrays(1, &vao); // get an vao id
  glBindVertexArray(vao); // activate it

  // 2. pass triangle vertices index by Element Buffer object
  vector<GLuint> triangle_indixes;
  auto offset = mesh.get_vertices().begin();
  for (const Mesh::Triangle& t : mesh.get_triangles())
  {
    // We should possibly use id in mesh.triangles ... instead of iterator
    triangle_indixes.push_back(t.a - offset);
    triangle_indixes.push_back(t.b - offset);
    triangle_indixes.push_back(t.c - offset);
  }
  unsigned int element_count = triangle_indixes.size();
  GLuint ebo;
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, // target
    element_count * sizeof(GLuint), // size of the buffer
    &(triangle_indixes[0]), // array to Initialize
    GL_STATIC_DRAW);

  // 3. pass coordinate and colors by creating and linking buffer objects
  vector<GLfloat> mesh_prop;
  for (const auto& v : mesh.get_vertices())
  {
    // coordinate (transformed by model and camera)
    // TODO push it at GPU
    Vec4 coord = v.coord * trans * camera->get_w2n();
    mesh_prop.push_back(coord.x);
    mesh_prop.push_back(coord.y);
    mesh_prop.push_back(coord.z);
    mesh_prop.push_back(coord.h);
    // color
    mesh_prop.push_back(v.color.r);
    mesh_prop.push_back(v.color.g);
    mesh_prop.push_back(v.color.b);
    mesh_prop.push_back(v.color.a);
  }
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh_prop.size(),
    &(mesh_prop[0]), GL_STATIC_DRAW);
  // link to attribute location
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,
    8 * sizeof(GLfloat), (GLvoid *)0);
  glEnableVertexAttribArray(0); // coordinate
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
    8 * sizeof(GLfloat), (GLvoid*)(4 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1); // color

  // TODO
  // pass a uniform transform matrix

  // Draw them
  glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, nullptr);

  // Delete after using
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ebo);
}


} // namespace CEL
