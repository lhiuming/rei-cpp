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
  this->width = width;
  this->height = height;
  buffer_maker.reserve(width * height * 4);
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
  // TODO: do brute-force rasterization
  // TODO: add depth buffer
  // TODO: do fast-exclude raseterization

  // transform to normalized coordinate
  Mat4 w2n = camera->get_w2n();
  Vec3 v0 = w2n * tri.a->coord;
  Vec3 v1 = w2n * tri.b->coord;
  Vec3 v2 = w2n * tri.c->coord;

  cout << "draw triangle (normalized): " << endl;
  cout << v0 << ", " << v1 << ", " << v2 << endl;

  // make screen coordinates
  // TODO: we might need to invert x or y axis
  float x0 = (v0.x + 1.0) / 2 * width, y0 = (v0.y + 1.0) / 2 * height;
  float x1 = (v1.x + 1.0) / 2 * width, y1 = (v1.y + 1.0) / 2 * height;
  float x2 = (v2.x + 1.0) / 2 * width, y2 = (v2.y + 1.0) / 2 * height;

  // define edge equations
  // TODO: do this butter with callable objects
  float dx0 = x1 - x0, dy0 = y1 - y0;
  float dx1 = x2 - x1, dy1 = y2 - y1;
  float dx2 = x0 - x2, dy2 = y0 - y2;
  float c0 = - x0 * dy0 + y0 * dx0;
  float c1 = - x1 * dy1 + y1 * dx1;
  float c2 = - x2 * dy2 + y2 * dx2;
  #define E0(x, y) ( dy0 * x - dx0 * y + c0 <= 0.0f )
  #define E1(x, y) ( dy1 * x - dx1 * y + c1 <= 0.0f )
  #define E2(x, y) ( dy2 * x - dx2 * y + c2 <= 0.0f )

  // Define the enclosing block
  int l = floor(min(x0, min(x1, x2))), r = ceil(max(x0, max(x1, x2)));
  int b = floor(min(y0, min(y1, y2))), t = ceil(max(y0, max(y1, y2)));

  for (float y = b + 0.5f; y < t; ++y)
    for (float x = l + 0.5f; x < r; ++x) {
      if ( E0(x, y) && E1(x, y) && E2(x, y) )
        put_sample(floor(x), floor(y));
    }

} // end rasterize_triangle

inline void SoftRenderer::put_sample(int x, int y)
{
  if (x < 0 || x >= width) return;
  if (y < 0 || x >= height) return;

  BufferSize offset = (y * width + x) * 4;
  buffer[offset    ] = (unsigned char) 255;
  buffer[offset + 1] = (unsigned char) 128;
  buffer[offset + 2] = (unsigned char) 128;
  buffer[offset + 3] = (unsigned char) 255;
}


} // namespace CEL
