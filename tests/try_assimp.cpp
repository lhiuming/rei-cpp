// Try the assimp model loader

#include <console.h>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

using CEL::console;

using namespace std;

const aiScene* as;

void print_face(const aiFace& face)
{
  console << "face has vertices ";
  for (int i = 0; i < face.mNumIndices; ++i)
  {
    console << face.mIndices[i] << " ";
  }
  console << endl;
}

void print_mesh(const aiMesh& mesh)
{
  console << "mesh has ";
  console << mesh.GetNumColorChannels() << " color chs, ";
  console << mesh.HasVertexColors(0) << " has color set, ";
  console << mesh.mNumFaces << " faces, ";
  console << mesh.mNumBones << " bones, ";
  console << mesh.mNumUVComponents[0] << " texture uv componets, ";
  console << mesh.mNumVertices << " vertices, ";
  console << mesh.mPrimitiveTypes << " primitive types";
  console << endl;

  // print each face
  for (int i = 0; i < mesh.mNumFaces; ++i) {
    console << "        ";
    print_face(mesh.mFaces[i]);
  }
  // sample vertex data
  if (mesh.mNumFaces > 0)
  {
    const aiFace& face0 = mesh.mFaces[0];
    console << "        " << "sample data (first vertex):" << endl;
    const aiVector3D& v = mesh.mVertices[face0.mIndices[0]];
    const aiVector3D& n = mesh.mNormals[face0.mIndices[0]];
    console << "        ";
    console << "coord: " << v.x << "," << v.y << "," << v.z << endl;
    console << "        ";
    console << "normal: " << n.x << "," << n.y << "," << n.z << endl;
  }
}

void check_model(const aiNode& node)
{
  // check the content of this node
  console << "Node has " << node.mNumMeshes << " meshes, ";
  console << node.mNumChildren << " children." << endl;

  // print the meshes if any
  for (int i = 0; i < node.mNumMeshes; ++i) {
    unsigned int mesh_ind = node.mMeshes[i];
    console << "    ";
    print_mesh(*(as->mMeshes[mesh_ind]));
  }

  // continue for all child nodes
  for(int a = 0; a < node.mNumChildren; ++a)
    check_model( *(node.mChildren[a]) );
}

int main(int argc, char** argv)
{
  // Check the input file name
  if (argc < 2) {
    console << "Not input .dae file." << endl;
    return -1;
  }
  string filename{argv[1]};
  console << "Filename is " << filename << endl;

  Assimp::Importer importer;

  // Read the cube.dae file
  if ( (as = importer.ReadFile(filename, 0)) ) {
    console << "Read .dae successfully" << endl;

    console << "The scene containts ";
    console << as->mNumCameras << " cameras, ";
    console << as->mNumLights << " lights, ";
    console << as->mNumMaterials << " materials, ";
    console << as->mNumMeshes << " meshes, ";
    console << as->mNumTextures << " textures";
    console << endl;

    console << "Ready to check the model : " << endl;
    check_model(*(as->mRootNode));
  } else {
    console << "Read .dae failed" << endl;
  }

  return 0;
}
