// source of renderer.h
#include "renderer.h"

#include <iostream>
#include <typeinfo>

using namespace std;

namespace CEL {

// Renderer ///////////////////////////////////////////////////////////////////
////

// Default constructor
Renderer::Renderer()
{
  cout << "A empty renderer is created. " << endl;
}

// Soft Renderer //////////////////////////////////////////////////////////////
////

// Default constructor
SoftRenderer::SoftRenderer() : Renderer() {}

// Set buffer size
void SoftRenderer::set_buffer_size(BufferSize width, BufferSize height)
{
  this->width = width; this->height = height;
  buffer.reserve(width * height * 4);
}

// Set draw function
void SoftRenderer::set_draw_func(draw_func fp)
{
  //this->f = fp;
}

// Render requiest
void SoftRenderer::render()
{
  // Make sure the scene and camera is set
  if (scene == nullptr) {
    cerr << "SoftRenderer Error: no scene! " << endl;
    return;
  }
  if (camera == nullptr) {
    cerr << "SoftRenderer Error: no camera! " << endl;
    return;
  }

  // Fetch all models
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

  cout << "done rendering" << endl;
}

// Rasterize a mesh
void SoftRenderer::rasterize_mesh(const Mesh& mesh, const Mat4& trans)
{
  // distribute the work load of rendering each triangle
  for (const auto& tri : mesh.get_triangles())
    rasterize_triangle(tri, trans);
}

// Rasterize a triangle
void SoftRenderer::rasterize_triangle(
  const Mesh::Triangle& tri, const Mat4& trans)
{
  cout << "draw triangle: ";
  cout << tri.a->coord << ", ";
  cout << tri.b->coord << ", ";
  cout << tri.c->coord << endl;
}


} // namespace CEL
