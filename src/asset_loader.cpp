// source of asset_loader.h
#include "asset_loader.h"

#include <iostream>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

using namespace std;

namespace CEL {

std::vector<MeshPtr>
AssetLoader::load_mesh(const std::string filename)
{
  // Try to load the file by assimp
  Assimp::Importer importer;
  const aiScene* as;
  if ( (as = importer.ReadFile(filename, 0)) == nullptr)
  {
    cout << "Read " << filename << " failed." << endl;
    return vector<MeshPtr>();
  }
  cout << "Read " << filename << " successfully. Trying to convert." << endl;
  cout << "  " << "File has " << as->mNumMeshes << " meshes in a scene" << endl;

  // read the mesh from children, with corrent world-space stransform
  std::vector<MeshPtr> ret;
  int read_count = load_mesh(as, as->mRootNode, ret, Mat4::I());
  cout << "  " << "Actually " << read_count << " mehses are read." << endl;

  return ret;
}


int AssetLoader::load_mesh(const aiScene* as, const aiNode* node,
  vector<MeshPtr>& models, Mat4 trans)
{
  int mesh_count = 0;

  // Add the mesh, with accumulated transform
  trans = trans * make_Mat4(node->mTransformation);
  for (int i = 0; i < node->mNumMeshes; ++i)
  {
    // Convert the aiMesh stored in aiScene
    unsigned int mesh_ind = node->mMeshes[i];
    models.push_back( make_mesh(*(as->mMeshes[mesh_ind]), trans) );
    ++mesh_count;
  }

  // Check the children
  for (int i = 0; i < node->mNumChildren; ++i)
    mesh_count += load_mesh(as, node->mChildren[i], models, trans);

  return mesh_count;
}


// transform a aiMatrix4x4 (from assimp) to a CEL::Mat4
Mat4 AssetLoader::make_Mat4(const aiMatrix4x4& mat)
{
  Mat4 ret;
  // aiMatrix4x4 {a1, a2, a3 ... } is row-major
  ret[0] = Vec4(mat.a1, mat.b1, mat.c1, mat.d1); // fill by cols
  ret[1] = Vec4(mat.a2, mat.b2, mat.c2, mat.d2);
  ret[2] = Vec4(mat.a3, mat.b3, mat.c3, mat.d3);
  ret[3] = Vec4(mat.a4, mat.b4, mat.c4, mat.d4);
  return ret;
}


MeshPtr AssetLoader::make_mesh(const aiMesh& mesh, Mat4 trans)
{
  // Convert all vertex
  vector<Mesh::Vertex> va;
  for (int i = 0; i < mesh.mNumVertices; ++i)
  {
    // convert and transform coordinate
    const aiVector3D& v = mesh.mVertices[i];
    Vec4 coord = trans * Vec4(v.x, v.y, v.z, 1.0);

    // convert color if any. NOTE: the mesh may containt multiple color set
    const int color_set = 0;
    Color color {0.0, 0.0, 0.6, 1.0};
    if (mesh.mColors[0] != NULL) {
      const aiColor4D& c = mesh.mColors[color_set][i];
      Color color (c.r, c.g, c.b, c.a);
    }

    // push the vertex
    va.push_back( Mesh::Vertex(coord, color) );

  } // end for

  // Convert all triangle faces
  using Triangle = typename Mesh::IndexTriangle;
  vector<Triangle> ta;
  for (int i = 0; i < mesh.mNumFaces; ++i)
  {
    const aiFace& face = mesh.mFaces[i];
    assert(face.mNumIndices == 3);
    Triangle it {face.mIndices[0], face.mIndices[1], face.mIndices[2]};
    ta.push_back(it);
  }

  return make_shared<Mesh>(std::move(va), std::move(ta));
}


} // namespace CEL
