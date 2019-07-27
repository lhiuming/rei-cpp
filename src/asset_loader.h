#ifndef REI_ASSET_LOADER_H
#define REI_ASSET_LOADER_H

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "algebra.h"
#include "camera.h"
#include "scene.h"

/**
 * asset_loader.h
 * Define a loader to load model from .dae files, and return the model in
 * OpenGL-style right-hand coordinates (+x right, +y up, -z forward).
 *
 * NOTE: Implementation details are hided in the source file, to reduce header
 * dependency.
 */

namespace rei {

using MeshPtr = std::shared_ptr<Mesh>;
using ScenePtr = std::shared_ptr<Scene>;
using CameraPtr = std::shared_ptr<Camera>;
using LightPtr = std::shared_ptr<void>;

// Forward declaration for the implemetation class
class AssimpLoaderImpl;

// The asset loader interface
class AssetLoader {
public:
  // Default constructor
  AssetLoader();

  // Load all models from the file
  std::vector<MeshPtr> load_meshes(const std::string filename);

  // Load the while 3D file as (scene, camera, lights)
  std::tuple<ScenePtr, CameraPtr, std::vector<LightPtr> > load_world(const std::string filename);

private:
  std::shared_ptr<AssimpLoaderImpl> impl;
};

} // namespace rei

#endif
