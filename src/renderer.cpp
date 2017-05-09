// source of renderer.h
#include "renderer.h"

#include <cstring>
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

// Set draw function
void SoftRenderer::set_draw_func(DrawFunc lambda_object)
{
  this->draw = lambda_object;
}

// Set buffer size
void SoftRenderer::set_buffer_size(BufferSize width, BufferSize height)
{
  this->width = width;
  this->height = height;
  buffer_maker.reserve(width * height * 3);
  pixels = &buffer_maker[0];
}

// Render request
void SoftRenderer::render()
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

  // TODO
  // Transform the scene into internal format (ready for shading)

  // TODO
  // Do vertex shading on the scene (internal format)

  // TODO
  // Project primitives by camera transforms
  // May be use a per-triangle function from this step.

  // TODO
  // Clipping against unit cube

  // TODO
  // screen mapping

  // TODO
  // pritimive(triangle) tasterizing (triangle traversal)
  // must write a helper function for this

  // clear the buffer
  memset(pixels, 0, width * height * 3 * sizeof(unsigned char));

  // Fetch and render all models
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

  // Draw on the screen
  draw(pixels, width, height);

}

// Rasterize a mesh
void SoftRenderer::rasterize_mesh(const Mesh& mesh, const Mat4& trans)
{
  // distribute the work load of rendering each triangle
  for (const auto& tri : mesh.get_triangles())
    rasterize_triangle(tri, trans);
}

// Rasterize a triangle
void
SoftRenderer::rasterize_triangle(const Mesh::Triangle& tri, const Mat4& trans)
{
  // TODO: add naive culling (exlucde triangles)
  // TODO: add depth buffer
  // TODO: do fast-exclude raseterization
  // TODO: add sopisticated culling

  // Naive culling
  if ( ! camera->visible(tri.a->coord) ) return;
  if ( ! camera->visible(tri.b->coord) ) return;
  if ( ! camera->visible(tri.b->coord) ) return;

  // transform to normalized coordinate
  const Mat4& w2n = camera->get_w2n();
  Vec3 v0 = w2n * tri.a->coord;
  Vec3 v1 = w2n * tri.b->coord;
  Vec3 v2 = w2n * tri.c->coord;

  // make screen coordinates
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
  auto E0 = [=](float x, float y) -> bool { return dy0 * x - dx0 * y + c0 <= 0.0f; };
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
  // TODO: we may discard this if we have culling
  if (x < 0 || x >= width) return;
  if (y < 0 || y >= height) return;

  BufferSize offset = (y * width + x) * 3;
  pixels[offset    ] = (unsigned char) 255;
  pixels[offset + 1] = (unsigned char) 128;
  pixels[offset + 2] = (unsigned char) 128;
}


} // namespace CEL
