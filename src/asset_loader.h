#ifndef CEL_ASSET_LOADER_H
#define CEL_ASSET_LOADER_H

#include <memory>
#include <vector>
#include <string>

#include "algebra.h"
#include "model.h"

/**
 * asset_loader.h
 * Define a loader to load model from .dae files, and return the model in
 * OpenGL-style right-hand coordinates (+x right, +y up, -z forward).
 *
 * Note: Implementation details are hided in the source file, to reduce header
 * dependency.
 */

namespace CEL {

using MeshPtr = std::shared_ptr<Mesh>;

class AssetLoader {
public:

  std::vector<MeshPtr> load_mesh(const std::string filename);

};

} // namespace CEL

#endif
