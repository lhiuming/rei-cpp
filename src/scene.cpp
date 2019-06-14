// source of scene.h
#include "scene.h"

#include <memory>
#include <sstream>
#include <vector>

#include "algebra.h"
#include "model.h"

using namespace std;

namespace CEL {

// Static scene ///////////////////////////////////////////////////////////////
////

string StaticScene::summary() const {
  ostringstream oss;
  oss << "Scene name: " << name << ", with " << models.size() << " models" << endl;
  for (const auto& mi : models) {
    oss << "  " << *(mi.pmodel);
    oss << "  with trans: " << mi.transform << endl;
  }

  return oss.str();
}

} // namespace CEL
