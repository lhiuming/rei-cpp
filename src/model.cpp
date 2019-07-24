// source of model.h
#include "model.h"

#include <sstream>

#include "string_utils.h"

using std::endl;
using std::wstring;

namespace rei {

wstring Model::summary() const {
  std::wostringstream oss;
  oss << L"Mesh name: " << name;
  oss << "  material: " << material->name << endl;
  return oss.str();
}

} // namespace rei
