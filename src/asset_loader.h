#ifndef CEL_ASSET_LOADER_H
#define CEL_ASSET_LOADER_H

#include <memory>
#include <vector>
#include <string>

#include "algebra.h"
#include "model.h"

#include <assimp/scene.h>

/**
 * asset_loader.h
 * Define a loader to load model from .dae files.
 */

namespace CEL {

using MeshPtr = std::shared_ptr<Mesh>;

class AssetLoader {
public:

  std::vector<MeshPtr> load_mesh(const std::string filename);

private:

  // Implementation helpers //

  int load_mesh(const aiScene* as, const aiNode* node,
    std::vector<MeshPtr>& models, Mat4 trans);

  Mat4 make_Mat4(const aiMatrix4x4& mat);
  MeshPtr make_mesh(const aiMesh& mesh, Mat4 trans);

};

} // namespace CEL

#endif
