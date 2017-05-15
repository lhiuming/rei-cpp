// Try the assimp model loader

#include <iostream>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags


using namespace std;

const aiScene* as;

void print_mesh(const aiMesh& mesh)
{
  cout << "mesh has " << mesh.mNumFaces << " faces" << endl;
}

void check_model(const aiNode& node)
{
  // check the content of this node
  cout << "Node has " << node.mNumMeshes << " meshes, ";
  cout << node.mNumChildren << " children." << endl;

  // print the meshes if any
  for (int i = 0; i < node.mNumMeshes; ++i) {
    unsigned int mesh_ind = node.mMeshes[i];
    cout << "    ";
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
    cout << "Not enough arguments." << endl;
    return -1;
  }
  string filename = argv[1];
  cout << "Filename is " << filename << endl;

  Assimp::Importer importer;

  // Read the cube.dae file
  if ( (as = importer.ReadFile(filename, 0)) ) {
    cout << "Read .dae successfully" << endl;

    cout << "The scene containts ";
    cout << as->mNumCameras << " cameras, ";
    cout << as->mNumLights << " lights, ";
    cout << as->mNumMaterials << " materials, ";
    cout << as->mNumMeshes << " meshes, ";
    cout << as->mNumTextures << " textures";
    cout << endl;

    cout << "Ready to check the model : " << endl;
    check_model(*(as->mRootNode));
  }

  return 0;
}
