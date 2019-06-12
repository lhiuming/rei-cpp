#if OPENGL_ENABLED
// source of renderer.h (in opengl/)
#include "gl_renderer.h"

#include <cstring>
#include <typeinfo>
#include <stdexcept>

#include "../console.h"

using namespace std;

static const char* default_vertex_shader_text =  // vertiex shader source
"#version 410 core\n"
"struct Light {\n"
"  vec3 dir;\n"
"  //float pad;\n"
"  vec4 ambient;\n"
"  vec4 diffuse;\n"
"};\n"
"uniform ubPerFrame {\n"
"  mat4 VP;\n"
"  Light light;\n"
"};\n"
"uniform ubPerObject {\n"
"  mat4 world;\n"
"  mat4 normal_w; \n"
"  vec4 material_diffuse;\n"
"};\n"
"layout (location = 0) in vec4 vPosition;\n"
"layout (location = 1) in vec3 vNormal;\n"
"layout (location = 2) in vec4 vColor;\n"
"out vec4 color;\n"
"out vec3 normal;\n"
"void main() {\n"
"  gl_Position = vPosition * world * VP;\n"
"  color = vColor;\n"
"  normal = vec3( vec4(vNormal, 0.0) * normal_w );\n"
"  //normal = vec3(normal_w[2]);\n"
"}\n";

static const char* default_fragment_shader_text =  // fragment shader source
"#version 410 core\n"
"struct Light {\n"
"  vec3 dir;\n"
"  //float pad;\n"
"  vec4 ambient;\n"
"  vec4 diffuse;\n"
"};\n"
"uniform ubPerFrame {\n"
"  mat4 VP;\n"
"  Light light;\n"
"};\n"
"uniform ubPerObject {\n"
"  mat4 world;\n"
"  mat4 normal_w; \n"
"  vec4 material_diffuse;\n"
"};\n"
"in vec4 color;\n"
"in vec3 normal;\n"
"out vec4 fcolor;\n"
"void main() {\n"
"  float tint = dot(normalize(light.dir), normalize(normal));\n"
"  //if (tint < 0.3) tint = 0.0;\n"
"  //else if (tint > 0.6) tint = 1.0;\n"
"  fcolor = (material_diffuse * 0.7 + color * 0.3) \n"
"    * ( light.ambient + tint * light.diffuse);\n"
"  fcolor.a = 1.0;\n"
"  //fcolor.rgb = normalize(normal);\n"// debug
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
  // delete shader
  glDeleteProgram(program);

  // destructing bufferd models resouces
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

  // Create uniform buffers //
  // Per frame uniform
  glGenBuffers(1, &(this->perFrameBuffer));
  //glBindBuffer(GL_UNIFORM_BUFFER, this->perFrameBuffer);  // set uniform
  glBindBufferBase(GL_UNIFORM_BUFFER, perFrameBufferIndex, // bind to slot 0
    this->perFrameBuffer);
  GLuint ubpf_index = glGetUniformBlockIndex(this->program, "ubPerFrame");
  glUniformBlockBinding(this->program, ubpf_index, perFrameBufferIndex);
  // Per object uniform
  glGenBuffers(1, &(this->perObjectBuffer));
  //glBindBuffer(GL_UNIFORM_BUFFER, this->perObjectBuffer); // set uniforom
  glBindBufferBase(GL_UNIFORM_BUFFER, perObjectBufferIndex, // bind to slot 1
    this->perObjectBuffer);
  GLuint ubpo_index = glGetUniformBlockIndex(this->program, "ubPerObject");
  glUniformBlockBinding(this->program, ubpo_index, perObjectBufferIndex);

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
  // Initialize non-GL datas (materials, model transforms )
  BufferedMesh bm(mesh, trans);

  // 1. Create a vertex array object
  glGenVertexArrays(1, &(bm.meshVAO)); // get an vao id
  glBindVertexArray(bm.meshVAO); // activate it

  // 2. Send triangle vertices index by Element Buffer object
  vector<GLuint> triangle_indices;
  for (const Mesh::Triangle& it : mesh.get_triangles())
  {
    triangle_indices.push_back(it.a);
    triangle_indices.push_back(it.b);
    triangle_indices.push_back(it.c);
  }
  glGenBuffers(1, &(bm.meshIndexBuffer));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bm.meshIndexBuffer); // activate
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, // target
    bm.indices_num() * sizeof(GLuint), // size of the buffer
    &(triangle_indices[0]), // array to Initialize
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
    // normal
    mesh_prop.push_back(v.normal.x);
    mesh_prop.push_back(v.normal.y);
    mesh_prop.push_back(v.normal.z);
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
  int stride = 4 + 3 + 4;
  // position = location 0
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,
    stride  * sizeof(GLfloat), (GLvoid *)0);
  glEnableVertexAttribArray(0);
  // normal = location 1
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
    stride * sizeof(GLfloat), (GLvoid*)(4 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);
  // color = location 2
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE,
    stride * sizeof(GLfloat), (GLvoid*)(7 * sizeof(GLfloat)));
  glEnableVertexAttribArray(2);

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

  // Update shared uniform buffer data
  ubPerFrame ubpf_data{camera->get_w2n()};
  Light dir_light{ // static global directional light
    {0.2f, 0.5f, 1.0f}, // direction
    1.0f, // pad
    {0.0f, 0.0f, 0.3f, 1.0f},  // ambient
    {0.8f, 0.1f, 0.1f, 1.0f}   // diffuse
  };
  ubpf_data.light = dir_light;
  glBindBuffer(GL_UNIFORM_BUFFER, this->perFrameBuffer);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(ubPerFrame), &ubpf_data,
    GL_STATIC_DRAW);

  // Render all buffered meshes
  render_meshes();

}


void GLRenderer::render_meshes()
{
  // Activate and render each mesh
  for (auto& buffered_mesh : meshes)
  {
    // Activate the per-mesh objects
    glBindVertexArray(buffered_mesh.meshVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffered_mesh.meshIndexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffered_mesh.meshVertexBuffer);

    // Update per-object uniform (to send mesh material data)
    glBindBuffer(GL_UNIFORM_BUFFER, this->perObjectBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ubPerObject),
      &(buffered_mesh.meshUniformData),  // here is the data
      GL_STATIC_DRAW);

    // Draw them
    glDrawElements(GL_TRIANGLES, (GLuint)buffered_mesh.indices_num(),
      GL_UNSIGNED_INT, nullptr);
  }

}


} // namespace CEL

#endif // OPENGL_ENABLED
