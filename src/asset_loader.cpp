// source of asset_loader.h
#include "asset_loader.h"

#include <iostream>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include "console.h"

using namespace std;

namespace CEL {

// AssetLoaderImp ///////////////////////////////////////////////////////////
// Private Class to this modules. Effetively seperates the assimp header
// dependency from the interface.
////

// NOTE: works almost like a namespace :P
class AssetLoaderImp {
public:

  // Main Functions //

  // Load as meshes
  static vector<MeshPtr> load_meshes(const string filename);

private:

  // Helpers Functions //
  static MeshPtr make_mesh(const aiMesh& mesh, const Mat4 trans);
  static int collect_mesh(const aiScene* as, const aiNode* node,
    vector<MeshPtr>& models, Mat4 trans);

  // Utilities
  static Mat4 make_Mat4(const aiMatrix4x4& aim);

};


// Utilities ////

// Convert from siMatrix4x4 to CEL::Mat4
inline Mat4 AssetLoaderImp::make_Mat4(const aiMatrix4x4& aim) {
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


// Big Helpers Functions ////

// Convert a aiMesh to Mesh and return a shared pointer
MeshPtr AssetLoaderImp::make_mesh(const aiMesh& mesh, const Mat4 trans)
{
  // Some little check
  if (mesh.GetNumColorChannels() > 1)
    console << "AssetLoader Warning: mesh has multiple color channels" << endl;
  if (mesh.HasVertexColors(1))
    console << "AssetLoader Warning: mesh has multiple color sets" << endl;

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
  using Triangle = typename Mesh::IndexTriangle;
  vector<Triangle> ta;
  for (int i = 0; i < mesh.mNumFaces; ++i)
  {
    const aiFace& face = mesh.mFaces[i];
    assert(face.mNumIndices == 3);
    Triangle it {face.mIndices[0], face.mIndices[1], face.mIndices[2]};
    ta.push_back(it);
  }

  // Done
  return make_shared<Mesh>(std::move(va), std::move(ta));

} // end make_mesh

// Convert from asMesh to a CEL::Mesh, and add to `models` [out]
int AssetLoaderImp::collect_mesh(const aiScene* as, const aiNode* node,
  vector<MeshPtr>& models, Mat4 trans)
{
  int mesh_count = 0;

  // Add the mesh to `models`, with accumulated transform
  trans = make_Mat4(node->mTransformation) * trans;
  for (int i = 0; i < node->mNumMeshes; ++i)
  {
    // Convert the aiMesh stored in aiScene
    unsigned int mesh_ind = node->mMeshes[i];
    models.push_back( make_mesh(*(as->mMeshes[mesh_ind]), trans) );
    ++mesh_count;
  }

  // Continue to collect the child if any
  for (int i = 0; i < node->mNumChildren; ++i)
    mesh_count += collect_mesh(as, node->mChildren[i], models, trans);

  return mesh_count;

}


// Main Interfaces //

// Load all meshes into world-space
vector<MeshPtr> AssetLoaderImp::load_meshes(const string filename)
{
  // Load as assimp scene
  Assimp::Importer importer;
  const aiScene* as = importer.ReadFile(filename,
    aiProcess_Triangulate |  // break-down polygons
    aiProcess_JoinIdenticalVertices |  // join vertices
    aiProcess_SortByPType);  // what is this?
  if (as == nullptr) // Read failed
  {
    cout << "Read " << filename << " failed." << endl;
    return vector<MeshPtr>();
  }
  cout << "Read " << filename << " successfully. Trying to convert." << endl;
  cout << "  " << "File has " << as->mNumMeshes << " meshes in a scene" << endl;

  // Process the assimp scene
  vector<MeshPtr> ret;
  int read_count = collect_mesh(as, as->mRootNode, ret, Mat4::I());
  cout << "  " << "Totally " << read_count << " mehses are read." << endl;

  return ret;
}


// AssetLoader /////////////////////////////////////////////////////////////
// Member functions are all just wappers!
////


vector<MeshPtr> AssetLoader::load_meshes(const std::string filename)
{
  return AssetLoaderImp::load_meshes(filename);
}






} // namespace CEL
