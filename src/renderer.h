#ifndef CEL_RENDERER_H
#define CEL_RENDERER_H

#include <iostream>

/*
 * renderer.h
 * Define a dump renderer class (currently).
 */

namespace CEL {

class Renderer {
public:
  Renderer() {
    std::cout << "A renderer is created. " << std::endl;
  }

};

} // namespace CEL

#endif 
