#ifndef CEL_ASSET_LOADER_H
#define CEL_ASSET_LOADER_H

#include <memory>
#include <vector>
#include <string>

#include "model.h"

/**
* asset_loader.h
* Define a loader to load model from .dae files.
*/

namespace CEL {

  using MeshPtr = std::shared_ptr<Mesh>;

  class AssetLoader {
  public:

    std::vector<MeshPtr> load_mesh(const std::string filename);

  };

} // namespace CEL

#endif
