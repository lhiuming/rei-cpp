// source of renderer.h (in opengl/)
#include "gl_renderer.h"

#include <cstring>
#include <typeinfo>
#include <stdexcept>

#include "../console.h"

using namespace std;

static const char* default_vertex_shader_text =  // vertiex shader source
"#version 410 core\n"
"layout (location = 0) in vec4 vPosition;\n"
"layout (location = 1) in vec4 vColor;\n"
"out vec4 color;\n"
"void main() {\n"
"  gl_Position = vPosition;\n"
"  color = vColor;\n"
"}\n";

static const char* default_fragment_shader_text =  // fragment shader source
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
  // Really nothings to do
}

// INIT: receive context, then initialized GL objects
void GLRenderer::set_gl_context(GLFWwindow* w)
{
  window = w;
  compile_shader(); // must run after GL context is prepared
}

// INIT: compile default shaders
void GLRenderer::compile_shader()
{ // NOTE: must run after the GL context is initialized

  // NOTE: currently using hard-coded text
  // TODO: load shader from file !

  // Variables for compile error check
  GLint success;
  GLchar infoLog[512];

  // Compile the vertex shader
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &default_vertex_shader_text, nullptr);
  glCompileShader(vertexShader);
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) // check if compile pass
  {
    glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
    cerr << "Error: vertex shader compile failed:\n" << infoLog << endl;
    throw runtime_error("Default Vertex Shader compile FAILD");
  }

  // Compile the fragment shader. Similar above.
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &default_fragment_shader_text, nullptr);
  glCompileShader(fragmentShader);
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
    cerr << "Error: fragment shader compile failed:\n" << infoLog << endl;
    throw runtime_error("Default Fragment Shader compile FAILD");
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
    throw runtime_error("Default Shader linkage FAILD");
  }

  // Do not need them after set up the shader program
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

}


// Destructor
GLRenderer::~GLRenderer()
{
  // TODO destructing bufferd models resouces
  for (auto& bm : meshes)
  {
    glDeleteVertexArrays(1, &(bm.meshVAO));
    glDeleteBuffers(1, &(bm.meshIndexBuffer));
    glDeleteBuffers(1, &(bm.meshVertexBuffer));
  }

  console << "GLRenderer destroyed" << endl;
}


// Configurations //

// Set buffer size
void GLRenderer::set_buffer_size(BufferSize width, BufferSize height)
{
  // Really nothings to do
}

// Set scene to renderer
void GLRenderer::set_scene(shared_ptr<const Scene> scene)
{ // Set the scene and buffer each model

  // Set
  Renderer::set_scene(scene);

  // Buffer each mesh
  // NOTE: only supports Mesh currently
  for (const auto& mi : scene->get_models() )
  {
    const Mesh& mesh = dynamic_cast<Mesh&>(*(mi.pmodel));
    const Mat4& trans = mi.transform;
    add_buffered_mesh(mesh, trans);
  }

}

void GLRenderer::add_buffered_mesh(const Mesh& mesh, const Mat4& trans)
{
  BufferedMesh bm(mesh);

  // 1. Create a vertex array object
  glGenVertexArrays(1, &(bm.meshVAO)); // get an vao id
  glBindVertexArray(bm.meshVAO); // activate it

  // 2. Send triangle vertices index by Element Buffer object
  vector<GLuint> triangle_indixes;
  for (const Mesh::IndexTriangle& it : mesh.get_indices())
  {
    triangle_indixes.push_back(it.a);
    triangle_indixes.push_back(it.b);
    triangle_indixes.push_back(it.c);
  }
  glGenBuffers(1, &(bm.meshIndexBuffer));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bm.meshIndexBuffer); // activate
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, // target
    bm.indices_num() * sizeof(GLuint), // size of the buffer
    &(triangle_indixes[0]), // array to Initialize
    GL_STATIC_DRAW);

  // 3. pass coordinate and colors by creating and linking buffer objects
  vector<GLfloat> mesh_prop;
  for (const auto& v : mesh.get_vertices())
  {
    // coordinate
    mesh_prop.push_back(v.coord.x);
    mesh_prop.push_back(v.coord.y);
    mesh_prop.push_back(v.coord.z);
    mesh_prop.push_back(v.coord.h);
    // color
    mesh_prop.push_back(v.color.r);
    mesh_prop.push_back(v.color.g);
    mesh_prop.push_back(v.color.b);
    mesh_prop.push_back(v.color.a);
  }
  glGenBuffers(1, &(bm.meshVertexBuffer));
  glBindBuffer(GL_ARRAY_BUFFER, bm.meshVertexBuffer);  // activate
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh_prop.size(),
    &(mesh_prop[0]), GL_STATIC_DRAW);
  // link to attribute location to match the layout
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,
    8 * sizeof(GLfloat), (GLvoid *)0);
  glEnableVertexAttribArray(0); // coordinate
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
    8 * sizeof(GLfloat), (GLvoid*)(4 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1); // color

  // Push at meshes
  meshes.push_back(bm);

} // end add_buffered_mesh

// Render request
void GLRenderer::render()
{
  // Make sure the scene and camera is set
  if (scene == nullptr) {
    cerr << "GLRenderer Error: no scene! " << endl;
    throw runtime_error("GLRenderer Error: no scene!");
  }
  if (camera == nullptr) {
    cerr << "GLRenderer Error: no camera! " << endl;
    throw runtime_error("GLRenderer Error: no camera");
  }

  // Activate shader programs
  glUseProgram(this->program);

  // Render all buffered models
  for (auto& bm : meshes)
    render_mesh(bm);

}


void GLRenderer::render_mesh(BufferedMesh& buffered_mesh)
{
  // Activate the per-mesh objects
  glBindVertexArray(buffered_mesh.meshVAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffered_mesh.meshIndexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffered_mesh.meshVertexBuffer);

  // TODO : send WVP transform to GPU (uniform shader variable )

  // Draw them
  glDrawElements(GL_TRIANGLES, (unsigned int)buffered_mesh.indices_num(),
   GL_UNSIGNED_INT, nullptr);

}


} // namespace CEL
