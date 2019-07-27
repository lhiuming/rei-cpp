// source of asset_loader.h
#include "asset_loader.h"

#include <assimp/postprocess.h> // Post processing flags
#include <assimp/scene.h>       // Output data structure
#include <assimp/Importer.hpp>  // C++ importer interface

#include "console.h"
#include "string_utils.h"

using namespace std;

namespace rei {

// AssimpLoaderImpl //////////////////////////////////////////////////////////
// Private Class to this modules. Effectively separates the assimp header
// dependency from the interface.
////

// NOTE: works almost like a namespace :P
class AssimpLoaderImpl {
public:
  // Default constructor
  AssimpLoaderImpl() : importer() {}

  // Main Functions //

  // Load the aiScene and check things
  int load_file(const string filename);

  // Return meshes
  vector<MeshPtr> load_meshes();

  // Return Scene, Camera and lights
  ScenePtr load_scene();
  CameraPtr load_camera();
  vector<LightPtr> load_lights(); // TODO

private:
  Assimp::Importer importer;
  const aiScene* as; // current loaded aiScene
  vector<Material> materials_list;

  // Helpers Functions //

  tuple<const aiNode*, Mat4> find_node(const aiNode* root, const string& node_name);

  int collect_mesh(const aiNode* node, vector<MeshPtr>& m_models, Mat4 trans);
  Model load_model(const aiNode& node, Mat4 scene_trans);

  // Utilities
  static Vec3 make_Vec3(const aiVector3D& v);
  static Mat4 make_Mat4(const aiMatrix4x4& aim);
  static Material make_material(const aiMaterial&);
  static MeshPtr make_mesh(const aiMesh& mesh, const Mat4 trans, const vector<Material>& maters);
};

// Main Interfaces //

// Load the file as aiScene
int AssimpLoaderImpl::load_file(const string filename) {
  // Load as assimp scene
  this->as = importer.ReadFile(filename,
    aiProcess_Triangulate |             // break-down polygons
      aiProcess_JoinIdenticalVertices | // join vertices
      aiProcess_SortByPType             // FIXME: what is this?
  );
  if (as == nullptr) // Read failed
  {
    console << "AssetLoader Error: Read " << filename << " FAILED." << endl;
    return -1;
  }
  console << "Read " << filename << " successfully." << endl;

  // Check contents //

  console << "File contents: " << endl;

  // Nodes
  const aiNode& root_node = *(as->mRootNode);
  console << "  Primary Nodes(" << root_node.mNumChildren << "): ";
  console << root_node.mChildren[0]->mName.C_Str();
  for (int i = 1; i < root_node.mNumChildren; ++i)
    console << ", " << root_node.mChildren[i]->mName.C_Str();
  console << endl;

  // Basic nums
  console << "  Meshes: " << as->mNumMeshes << endl;
  console << "  Cameras: " << as->mNumCameras << endl;
  console << "  Lights: " << as->mNumLights << endl;
  console << "  Materials: " << as->mNumMaterials << endl;

  // Load common material list
  for (int i = 0; i < as->mNumMaterials; ++i)
    this->materials_list.push_back(make_material(*(as->mMaterials[i])));

  return 0;
}

// Load all meshes into world-space
vector<MeshPtr> AssimpLoaderImpl::load_meshes() {
  // Collect meshes
  vector<MeshPtr> ret;
  int read_count = collect_mesh(as->mRootNode, ret, Mat4::I());
  console << "Loaded meshes : " << read_count << endl;

  return ret;
}

// Load as REI::Scene
ScenePtr AssimpLoaderImpl::load_scene() {
  const aiNode& root_node = *(as->mRootNode);
  auto ret = make_shared<Scene>(make_wstring(root_node.mName.C_Str()));

  // Load model from each child nodes
  Mat4 coordinate_trans = make_Mat4(root_node.mTransformation);
  for (int i = 0; i < root_node.mNumChildren; ++i) {
    const aiNode& child = *(root_node.mChildren[i]);
    string name {child.mName.C_Str()};
    if (name == "Camera") {
      console << "AssetLoader: Skip a Camera node in scene";
      console << "(" << child.mNumMeshes << " meshes)" << endl;
    } else if (name == "Light") {
      console << "AssetLoader: Skip a Light node in scene";
      console << "(" << child.mNumMeshes << " meshes)" << endl;
    } else {
      // Load the model without instance-transform
      auto mi = load_model(child, coordinate_trans);
      if (mi.get_geometry()) // check if no real mesh loaded
        ret->add_model(std::move(mi));
    }
  }

  return ret;
}

CameraPtr AssimpLoaderImpl::load_camera() {
  // Check numbers of camera
  if (as->mNumCameras < 1) {
    console << "AssetLoader Warning: No Camera. Use default." << endl;
    return make_shared<Camera>(Vec3 {0, 0, 10}, Vec3 {0, 0, -1});
  }
  if (as->mNumCameras > 1)
    console << "AssetLoader Warning: Multiple camera. Use the first." << endl;

  // Find the node of the first camera (for transform)
  const aiCamera& cam = *(as->mCameras[0]);
  auto cam_node = find_node(as->mRootNode, cam.mName.C_Str());
  if (!get<0>(cam_node)) console << "AssetLoader Wranining: failed to find the camera node" << endl;

  // Convert the first camera
  Mat4& trans = get<1>(cam_node);
  Vec3 pos = Vec4(make_Vec3(cam.mPosition), 1.0) * trans;
  Vec3 dir = make_Vec3(cam.mLookAt) * trans.sub3();
  Vec3 up = make_Vec3(cam.mUp) * trans.sub3();
  auto ret = make_shared<Camera>(pos, dir, up);
  ret->set_params(cam.mAspect,
    cam.mHorizontalFOV * (180.0 / 3.14), // radian -> degree
    cam.mClipPlaneNear, cam.mClipPlaneFar);

  console << "Loaded camera : " << cam.mName.C_Str() << endl;
  return ret;
}

// Big Helpers Functions ////

// Find the node with given name; return the node and accumulated transform.
tuple<const aiNode*, Mat4> AssimpLoaderImpl::find_node(
  const aiNode* root, const string& node_name) {
  // Find the node with given name, starting from the root

  // Check name of this node
  string root_name = root->mName.C_Str();
  if (root_name == node_name) { return make_tuple(root, make_Mat4(root->mTransformation)); }

  // Check child nodes
  for (int i = 0; i < root->mNumChildren; ++i) {
    auto ret = find_node(root->mChildren[i], node_name);
    if (get<0>(ret) != nullptr) {
      get<1>(ret) = get<1>(ret) * make_Mat4(root->mTransformation);
      return ret;
    }
  }

  // not found
  return make_tuple(nullptr, Mat4::I());
}

// Convert from asMesh to a REI::Mesh, and add to `models` [out]
int AssimpLoaderImpl::collect_mesh(const aiNode* node, vector<MeshPtr>& m_models, Mat4 trans) {
  int mesh_count = 0;

  // Add the mesh to `models`, with accumulated transform
  trans = make_Mat4(node->mTransformation) * trans;
  for (int i = 0; i < node->mNumMeshes; ++i) {
    // Convert the aiMesh stored in aiScene
    unsigned int mesh_ind = node->mMeshes[i];

    m_models.push_back(make_mesh(*(as->mMeshes[mesh_ind]), trans, materials_list));
    ++mesh_count;
  }

  // Continue to collect the child if any
  for (int i = 0; i < node->mNumChildren; ++i)
    mesh_count += collect_mesh(node->mChildren[i], m_models, trans);

  return mesh_count;
}

Model AssimpLoaderImpl::load_model(const aiNode& node, Mat4 coordinate_trans) {
  // Correct the world-transfor to fit in right-hand coordinate_trans
  Mat4 old_W = make_Mat4(node.mTransformation);
  Mat4 W = coordinate_trans.inv() * old_W * coordinate_trans;

  MeshPtr mesh = nullptr;

  // Build Hiearchy model
  if (node.mNumChildren > 0) {
    // TODO : make Hiearchy
    console << "AssetLoader Warnning: need hiearachy";
    console << "(name = " << node.mName.C_Str() << "chile = " << node.mNumChildren << ")" << endl;
    return Model {L"empty", W, nullptr, nullptr};
  }

  // Build Non Hiearchy model
  if (node.mNumMeshes < 1) // Nothing here
  {
    console << "AssetLoader Warnning: empty node";
    console << "(name = " << node.mName.C_Str() << ")" << endl;
  } else if (node.mNumMeshes == 1) // Mesh
  {
    // Make a mesh fron aiMesh
    REI_NOT_IMPLEMENTED
    mesh = make_mesh(*(as->mMeshes[node.mMeshes[0]]), coordinate_trans, this->materials_list);
    console << "AssetLoader: loaded node name = " << node.mName.C_Str() << ")" << endl;
  } else // Mesh aggrerate
  {
    // TODO
    console << "AssetLoader Warnning: need aggregate";
    console << "(name = " << node.mName.C_Str() << ", meshnum = " << node.mNumMeshes << ")" << endl;
  }

  // Return an instance
  return Model {L"assimp no name", W, mesh, nullptr};
}

// Utilities ////

// Convert from aiVector3D to REI::Vec3
inline Vec3 AssimpLoaderImpl::make_Vec3(const aiVector3D& v) {
  return Vec3(v.x, v.y, v.z);
}

// Convert from aiMatrix4x4 to REI::Mat4
inline Mat4 AssimpLoaderImpl::make_Mat4(const aiMatrix4x4& aim) {
  // NOTE: aiMatrix4x4 {a1, a2, a3 ... } is row-major,
  // So transposed here to fit column-major Mat4
  return Mat4(aim.a1, aim.b1, aim.c1, aim.d1, aim.a2, aim.b2, aim.c2, aim.d2, aim.a3, aim.b3,
    aim.c3, aim.d3, aim.a4, aim.b4, aim.c4, aim.d4);
}

// Convert all aiMaterial to Mesh::Material
Material AssimpLoaderImpl::make_material(const aiMaterial& mater) {
  Material ret;

  // Material name
  aiString mater_name;
  mater.Get(AI_MATKEY_NAME, mater_name);
  // ret.name = string(mater_name.C_Str());

  // Shading model
  int shading_model;
  mater.Get(AI_MATKEY_SHADING_MODEL, shading_model);

  // Key-Value pairs
  aiColor3D dif(0.f, 0.f, 0.f);
  aiColor3D amb(0.f, 0.f, 0.f);
  aiColor3D spec(0.f, 0.f, 0.f);
  float shine = 0.0;

  mater.Get(AI_MATKEY_COLOR_AMBIENT, amb);
  mater.Get(AI_MATKEY_COLOR_DIFFUSE, dif);
  mater.Get(AI_MATKEY_COLOR_SPECULAR, spec);
  mater.Get(AI_MATKEY_SHININESS, shine);

  REI_NOT_IMPLEMENTED
  /*
  ret.diffuse = Color(dif.r, dif.g, dif.b);
  ret.ambient = Color(amb.r, amb.g, amb.b);
  ret.specular = Color(spec.r, spec.g, spec.b);
  ret.shineness = shine;
  */

  return ret;
}

// Convert a aiMesh to Mesh and return a shared pointer
MeshPtr AssimpLoaderImpl::make_mesh(
  const aiMesh& mesh, const Mat4 trans, const vector<Material>& mat_list) {
  // Some little check
  if (mesh.GetNumColorChannels() > 1)
    console << "AssetLoader Warning: mesh has multiple color channels" << endl;
  if (mesh.HasVertexColors(1))
    console << "AssetLoader Warning: mesh has multiple color sets" << endl;

  // Create the mesh with node name
  MeshPtr ret = make_shared<Mesh>(string(mesh.mName.C_Str()));

  // Convert all vertex (with coordinates, normals, and colors)
  vector<Mesh::Vertex> va;
  Mat3 trans_normal = trans.adj3();
  for (int i = 0; i < mesh.mNumVertices; ++i) {
    // Coordinates & Normal (store in world-space)
    const aiVector3D& v = mesh.mVertices[i];
    const aiVector3D& n = mesh.mNormals[i]; // NOTE: pertain scaling !
    Vec4 coord = Vec4(v.x, v.y, v.z, 1.0) * trans;
    Vec3 normal = (Vec3(n.x, n.y, n.z) * trans_normal).normalized();

    // Color
    // NOTE: the mesh may containt multiple color set
    const int color_set = 0;
    Color color;
    if (mesh.HasVertexColors(color_set)) {
      const aiColor4D& c = mesh.mColors[color_set][i];
      color = Color(c.r, c.g, c.b, c.a);
    } else {
      color = Color {0.5f, 0.5f, 0.5f, 1.0f};
    }

    // Push to vertex array
    va.push_back(Mesh::Vertex(coord, normal, color));

  } // end for all vertex

  // Encode all triangle (face) as vertex index
  using Triangle = typename Mesh::Triangle;
  vector<Mesh::Triangle> ta;
  for (int i = 0; i < mesh.mNumFaces; ++i) {
    const aiFace& face = mesh.mFaces[i];
    assert(face.mNumIndices == 3);
    Triangle it {face.mIndices[0], face.mIndices[1], face.mIndices[2]};
    ta.push_back(it);
  }

  // Set the data and material, then done
  ret->set(std::move(va), std::move(ta));
  REI_NOT_IMPLEMENTED
  // ret->set(mat_list[mesh.mMaterialIndex]); // fix model and mesh and material stuffs
  return ret;

} // end make_mesh

// AssetLoader /////////////////////////////////////////////////////////////
// Member functions are all just wappers!
////

// Default constructor; make a AssimpLoaderImpl
AssetLoader::AssetLoader() {
  this->impl = make_shared<AssimpLoaderImpl>();
}

// Load all models from the file
vector<MeshPtr> AssetLoader::load_meshes(const std::string filename) {
  impl->load_file(filename);
  return impl->load_meshes();
}

// Load the while 3D file as (scene, camera, lights)
tuple<ScenePtr, CameraPtr, std::vector<LightPtr> > AssetLoader::load_world(
  const std::string filename) {
  impl->load_file(filename);
  ScenePtr sp = impl->load_scene();
  CameraPtr cp = impl->load_camera();

  return make_tuple(sp, cp, std::vector<LightPtr>());
}

} // namespace rei
