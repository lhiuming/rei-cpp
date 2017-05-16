// source of renderer.h (in opengl/)
#include "gl_renderer.h"

#include <cstring>
#include <iostream>
#include <typeinfo>

using namespace std;

namespace CEL {

// Default constructor
GLRenderer::GLRenderer() : Renderer() {
  // TODO initializde the shader programs
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

  // TODO
  // Transform the scene into internal format (ready for shading)

  // TODO Do vertex shading on the scene (internal format)
  // 1. create vertex array object
  // 2. pass coordinate and colors by creating and linking buffer objects
  // 3. pass triangle vertices index by Element Buffer object


  // Fetch and render all models
  for (const auto& mi : scene->get_models() )
  {
    const Model& model = *(mi.pmodel);
    const Mat4& trans = mi.transform;

    // choose a rendering procedure
    const auto& model_type = typeid(model);
    if (model_type == typeid(Mesh)) {
      //rasterize_mesh(dynamic_cast<const Mesh&>(model), trans);
    } else {
      cerr << "Error: Unkown model type: "<< model_type.name() << endl;
    }
  } // end for

}

void GLRenderer::rasterize_mesh(const Mesh& mesh, const Mat4& trans)
{
  
}


} // namespace CEL
