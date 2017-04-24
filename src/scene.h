#ifndef CEL_SCENE_H
#define CEL_SCENE_H

#include <iostream>

/*
 * scene.h
 * Define a Scene class (dump, currently)
 */

namespace CEL {

class Scene {
public:
  Scene() {
    std::cout << "A scene is created. " << std::endl;
  }
};

} // namespace CEL

#endif
