// source of renderer.h
#include "renderer.h"

#include <iostream>

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
  auto& models = scene->get_models();

  for (int i = 0; i < models.size(); ++i)
  {
    const ModelPtr& p = models[i].pmodel;
    const Mat4& trans = models[i].transform;

    // TODO: need better way to get different methods..

  }

  cout << "done rendering" << endl;
}

// Rasterize a mesh
void SoftRenderer::rasterize_mesh(const Mesh& mesh, const Mat4& trans)
{

}

// Rasterize a triangle
void SoftRenderer::rasterize_triangle(const TriangleType& tri,
   const Mat4& trans)
{
  cout << "draw triangle: ";
  cout << tri.a->coord << ", ";
  cout << tri.b->coord << ", ";
  cout << tri.c->coord << endl;
}


} // namespace CEL
