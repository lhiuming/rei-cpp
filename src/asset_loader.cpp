// source of asset_loader.h
#include "asset_loader.h"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include "console.h"

using namespace std;

namespace CEL {

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
  vector<Mesh::Material> materials_list;

  // Helpers Functions //
  int collect_mesh(const aiNode* node, vector<MeshPtr>& models, Mat4 trans);
  ModelInstance load_model(const aiNode& node, Mat4 scene_trans);

  // Utilities
  static Mat4 make_Mat4(const aiMatrix4x4& aim);
  static Mesh::Material make_material(const aiMaterial&);
  static MeshPtr make_mesh(const aiMesh& mesh, const Mat4 trans,
    const vector<Mesh::Material>& maters);
};


// Main Interfaces //

// Load the file as aiScene
int AssimpLoaderImpl::load_file(const string filename)
{
  // Load as assimp scene
  this->as = importer.ReadFile(filename,
    aiProcess_Triangulate |            // break-down polygons
    aiProcess_JoinIdenticalVertices |  // join vertices
    aiProcess_SortByPType              // FIXME: what is this?
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
  console << "  Primary Nodes (" << root_node.mNumChildren << ") : ";
    console << root_node.mChildren[0]->mName.C_Str() << ", ";
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
vector<MeshPtr> AssimpLoaderImpl::load_meshes()
{
  // Collect meshes
  vector<MeshPtr> ret;
  int read_count = collect_mesh(as->mRootNode, ret, Mat4::I());

  return ret;
}

// Load as CEL::Scene ( StaticScene )
ScenePtr AssimpLoaderImpl::load_scene()
{
  // Put meshes
  const aiNode& root_node = *(as->mRootNode);
  Mat4 coordinate_trans = make_Mat4(root_node.mTransformation);
  auto ret = make_shared<StaticScene>();
  for (int i = 0; i < root_node.mNumChildren; ++i)
  {
    const aiNode& child = *(root_node.mChildren[i]);
    string name{ child.mName.C_Str() };
    if (name == "Camera") {
      console << "AssetLoader Warning: Skip a camera node in scene" << endl;
    } else if (name == "Light") {
      console << "AssetLoader Warning: Skip a light node in scene" << endl;
    } else {
      // Load the model
      ret->add_model( load_model(child, coordinate_trans));
    }
  }

  return ret;
}

CameraPtr AssimpLoaderImpl::load_camera()
{

}


// Big Helpers Functions ////


// Convert from asMesh to a CEL::Mesh, and add to `models` [out]
int AssimpLoaderImpl::collect_mesh(const aiNode* node,
  vector<MeshPtr>& models, Mat4 trans)
{
  int mesh_count = 0;

  // Add the mesh to `models`, with accumulated transform
  trans = make_Mat4(node->mTransformation) * trans;
  for (int i = 0; i < node->mNumMeshes; ++i)
  {
    // Convert the aiMesh stored in aiScene
    unsigned int mesh_ind = node->mMeshes[i];

    models.push_back(
      make_mesh(*(as->mMeshes[mesh_ind]), trans, materials_list)
    );
    ++mesh_count;
  }

  // Continue to collect the child if any
  for (int i = 0; i < node->mNumChildren; ++i)
    mesh_count += collect_mesh(node->mChildren[i], models, trans);

  return mesh_count;

}

ModelInstance
AssimpLoaderImpl::load_model(const aiNode& node, Mat4 scene_trans)
{
  if (node.mChildren > 0)
  {
    // TODO : make Hiearchy
    console << "AssetLoader Warnning: need hiearachy" << endl;
    return ModelInstance{};
  }

  if (node.mNumMeshes < 1) // No mesh
  {
    console << "AssetLoader Warnning: empty node" << endl;
    return ModelInstance{};
  }
  if (node.mNumMeshes == 1) // Plain Mesh
  {
    // Make a mesh
    ModelPtr mp = make_mesh(*(as->mMeshes[node.mMeshes[0]]),
      scene_trans, this->materials_list)
    // Return a instance
    Mat4 old_world_trans = make_Mat4(node->mTransformation);
    Mat4 true_world_trans = scene_trans.T() * old_world_trans * scene_trans;
    return ModelInstance{mp, true_world_trans};
  }
  else // Mesh aggrerate
  {
    // TODO
    console << "AssetLoader Warnning: need aggregate" << endl;
    return ModelInstance{};
  }


  // Return an instance
  return ModelInstance
}



// Utilities ////

// Convert from siMatrix4x4 to CEL::Mat4
inline Mat4 AssimpLoaderImpl::make_Mat4(const aiMatrix4x4& aim)
{
  Mat4 ret;
  // NOTE: aiMatrix4x4 {a1, a2, a3 ... } is row-major,
  // So transposed to fit column-maojr Mat4
  return Mat4(
    aim.a1, aim.b1, aim.c1, aim.d1,
    aim.a2, aim.b2, aim.c2, aim.d2,
    aim.a3, aim.b3, aim.c3, aim.d3,
    aim.a4, aim.b4, aim.c4, aim.d4
  );
}

// Convert all aiMaterial to Mesh::Material
Mesh::Material AssimpLoaderImpl::make_material(const aiMaterial& mater)
{
  Mesh::Material ret;

  // Material name
  aiString mater_name;
  mater.Get(AI_MATKEY_NAME, mater_name);
  ret.name = string(mater_name.C_Str());

  // Shading model
  int shading_model;
  mater.Get(AI_MATKEY_SHADING_MODEL, shading_model);

  // Key-Value pairs
  aiColor3D dif(0.f,0.f,0.f);
  aiColor3D amb(0.f,0.f,0.f);
  aiColor3D spec(0.f,0.f,0.f);
  float shine = 0.0;

  mater.Get(AI_MATKEY_COLOR_AMBIENT, amb);
  mater.Get(AI_MATKEY_COLOR_DIFFUSE, dif);
  mater.Get(AI_MATKEY_COLOR_SPECULAR, spec);
  mater.Get(AI_MATKEY_SHININESS, shine);

  ret.diffuse = Color(dif.r, dif.g, dif.b);
  ret.ambient = Color(amb.r, amb.g, amb.b);
  ret.specular = Color(spec.r, spec.g, spec.b);
  ret.shineness = shine;

  return ret;
}

// Convert a aiMesh to Mesh and return a shared pointer
MeshPtr AssimpLoaderImpl::make_mesh(const aiMesh& mesh, const Mat4 trans,
  const vector<Mesh::Material>& mat_list)
{
  // Some little check
  if (mesh.GetNumColorChannels() > 1)
    console << "AssetLoader Warning: mesh has multiple color channels" << endl;
  if (mesh.HasVertexColors(1))
    console << "AssetLoader Warning: mesh has multiple color sets" << endl;

  // Create the mesh with node name
  MeshPtr ret = make_shared<Mesh>(string(mesh.mName.C_Str()));

  // Convert all vertex (with coordinates, normals, and colors)
  vector<Mesh::Vertex> va;
  const Mat3 trans_normal = trans.sub3(); // don't use adj3(); you need scale
  for (int i = 0; i < mesh.mNumVertices; ++i)
  {
    // Coordinates & Normal (store in world-space)
    const aiVector3D& v = mesh.mVertices[i];
    const aiVector3D& n = mesh.mNormals[i]; // NOTE: pertain scaling !
    Vec4 coord = Vec4(v.x, v.y, v.z, 1.0) * trans;
    Vec3 normal = Vec3(n.x, n.y, n.z) * trans_normal;

    // Color
    // NOTE: the mesh may containt multiple color set
    const int color_set = 0;
    Color color;
    if (mesh.HasVertexColors(color_set))
    {
      const aiColor4D& c = mesh.mColors[color_set][i];
      color = Color(c.r, c.g, c.b, c.a);
    }
    else
    {
      color = Color{ 0.5f, 0.5f, 0.5f, 1.0f };
    }

    // Push to vertex array
    va.push_back( Mesh::Vertex(coord, normal, color) );

  } // end for all vertex

  // Encode all triangle (face) as vertex index
  using Triangle = typename Mesh::Triangle;
  vector<Mesh::Triangle> ta;
  for (int i = 0; i < mesh.mNumFaces; ++i)
  {
    const aiFace& face = mesh.mFaces[i];
    assert(face.mNumIndices == 3);
    Triangle it {face.mIndices[0], face.mIndices[1], face.mIndices[2]};
    ta.push_back(it);
  }

  // Set the data and material, then done
  ret->set(std::move(va), std::move(ta));
  ret->set(mat_list[mesh.mMaterialIndex]);
  return ret;

} // end make_mesh


// AssetLoader /////////////////////////////////////////////////////////////
// Member functions are all just wappers!
////

// Default constructor; make a AssimpLoaderImpl
AssetLoader::AssetLoader()
{
  this->impl = make_shared<AssimpLoaderImpl>();
}


// Load all models from the file
vector<MeshPtr> AssetLoader::load_meshes(const std::string filename)
{
  impl->load_file(filename);
  return impl->load_meshes();
}

// Load the while 3D file as (scene, camera, lights)
tuple< ScenePtr, CameraPtr, std::vector<LightPtr> >
AssetLoader::load_world(const std::string filename)
{
  impl->load_file(filename);
  ScenePtr sp = impl->load_scene();

  return make_tuple(sp, CameraPtr(), std::vector<LightPtr>());
}



} // namespace CEL
