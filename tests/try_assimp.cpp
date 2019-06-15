// Try the assimp model loader

#include <algebra.h>
#include <console.h>

#include <assimp/postprocess.h> // Post processing flags
#include <assimp/scene.h>       // Output data structure
#include <assimp/Importer.hpp>  // C++ importer interface

using namespace std;
using namespace rei;

void print_scene(const aiScene& as);
void print_material(const aiMaterial& mater, string pre, int idx);
void print_camera(const aiCamera& cam, string pre, int idx);
void print_node(const aiNode& node, string pre, const aiScene& as);
void print_mesh(const aiMesh& mesh, string pre, int idx);
void print_face(const aiFace& face, string pre, int idx);

// vector printer
ostream& operator<<(ostream& os, const aiVector3D& v) {
  return os << "vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
}

// matric printer
ostream& operator<<(ostream& os, const aiMatrix4x4& aim) {
  return os << Mat4(aim.a1, aim.b1, aim.c1, aim.d1, aim.a2, aim.b2, aim.c2, aim.d2, aim.a3, aim.b3,
           aim.c3, aim.d3, aim.a4, aim.b4, aim.c4, aim.d4);
}

int main(int argc, char** argv) {
  if (argc < 2) console << "No input model." << endl;

  string filename(argv[1]);

  Assimp::Importer importer;
  const aiScene* as = importer.ReadFile(filename,
    aiProcess_Triangulate |             // break-down polygons
      aiProcess_JoinIdenticalVertices | // join vertices
      aiProcess_SortByPType);           // what is this?
  if (as) {
    console << "Read path : " << filename << endl;
    print_scene(*as);
  }

  return 0;
}

void print_scene(const aiScene& as) {
  string pre = "";

  // print overview
  console << pre << "the scene contains ";
  console << as.mNumCameras << " cameras, ";
  console << as.mNumLights << " lights, ";
  console << as.mNumMaterials << " materials, ";
  console << as.mNumMeshes << " meshes, ";
  console << as.mNumTextures << " textures, ";
  console << as.mNumAnimations << " animation";
  console << endl;

  // print materials with index
  for (int i = 0; i < as.mNumMaterials; ++i)
    print_material(*(as.mMaterials[i]), pre, i);

  // print camera with index
  for (int i = 0; i < as.mNumCameras; ++i)
    print_camera(*(as.mCameras[i]), pre, i);

  // print from root node
  print_node(*(as.mRootNode), pre, as);
}

void print_material(const aiMaterial& mater, string pre, int idx) {
  pre += to_string(idx) + string("[material]");

  // Material name
  aiString mater_name;
  mater.Get(AI_MATKEY_NAME, mater_name);
  console << pre << "name: " << mater_name.C_Str() << endl;

  pre += string(" ");

  // Shading model
  int shading_model;
  mater.Get(AI_MATKEY_SHADING_MODEL, shading_model);
  switch (shading_model) {
    case aiShadingMode_Phong:
      console << pre << "Using Phone shaing;" << endl;
      break;
    case aiShadingMode_Gouraud:
      console << pre << "Using Gouraud shading;" << endl;
      break;
    default:
      console << pre << "Using unidentified shading;" << endl;
      break;
  }

  // Key-Value pairs
  aiColor3D dif(0.f, 0.f, 0.f);
  aiColor3D amb(0.f, 0.f, 0.f);
  aiColor3D spec(0.f, 0.f, 0.f);
  float shine = 0.0;

  mater.Get(AI_MATKEY_COLOR_AMBIENT, amb);
  mater.Get(AI_MATKEY_COLOR_DIFFUSE, dif);
  mater.Get(AI_MATKEY_COLOR_SPECULAR, spec);
  mater.Get(AI_MATKEY_SHININESS, shine);

  Vec3 diffuse(dif.r, dif.g, dif.b);
  Vec3 ambient(amb.r, amb.g, amb.b);
  Vec3 specular(spec.r, spec.g, spec.b);
  double shineness = shine;

  console << pre << "Diffuse: " << diffuse << endl;
  console << pre << "Ambient: " << ambient << endl;
  console << pre << "Specular: " << specular << endl;
  console << pre << "Shineness: " << shineness << endl;
}

void print_camera(const aiCamera& cam, string pre, int idx) {
  pre += string("[camera]");

  console << pre << "name: " << cam.mName.C_Str() << endl;
  console << pre << "  aspect: " << cam.mAspect << endl;
  console << pre << "  z-near = " << cam.mClipPlaneNear << ", "
          << "z-far = " << cam.mClipPlaneFar << endl;
  console << pre << "  angle: " << cam.mHorizontalFOV << endl;
  console << pre << "pos : " << cam.mPosition << endl;
  console << pre << "look at : " << cam.mLookAt << endl;
  console << pre << "up : " << cam.mUp << endl;
}

void print_node(const aiNode& node, string pre, const aiScene& as) {
  pre = pre + string("[n]");

  // check the content of this node
  console << pre << "name: " << node.mName.C_Str() << endl;
  console << pre << "  mesh num: " << node.mNumMeshes << endl;
  console << pre << "  child num:" << node.mNumChildren << endl;
  console << pre << "  node trans: " << node.mTransformation << endl;

  // print the meshes if any
  for (int i = 0; i < node.mNumMeshes; ++i) {
    unsigned int mesh_ind = node.mMeshes[i];
    print_mesh(*(as.mMeshes[mesh_ind]), pre, i);
  }

  // continue for all child nodes
  for (int a = 0; a < node.mNumChildren; ++a)
    print_node(*(node.mChildren[a]), pre, as);
}

void print_mesh(const aiMesh& mesh, string pre, int idx) {
  pre += to_string(idx) + string("[mesh]");

  console << pre << "mesh has ";
  console << mesh.GetNumColorChannels() << " color chs, ";
  console << mesh.HasVertexColors(0) << " has color set, ";
  console << mesh.mMaterialIndex << "-th material, ";
  console << mesh.mNumFaces << " faces, ";
  console << mesh.mNumBones << " bones, ";
  console << mesh.mNumUVComponents[0] << " texture uv componets, ";
  console << mesh.mNumVertices << " vertices, ";
  console << mesh.mPrimitiveTypes << " primitive types";
  console << endl;

  // print each face
  for (int i = 0; i < mesh.mNumFaces; ++i) {
    print_face(mesh.mFaces[i], pre, i);
  }
  // sample vertex data
  if (mesh.mNumFaces > 0) {
    const aiFace& face0 = mesh.mFaces[0];
    console << pre << "sample data:";
    console << "coord = " << mesh.mVertices[face0.mIndices[0]];
    console << "normal = " << mesh.mNormals[face0.mIndices[0]] << endl;
  }
}

void print_face(const aiFace& face, string pre, int idx) {
  pre += to_string(idx) + string("[face]");

  console << pre << "face has vertices ";
  for (int i = 0; i < face.mNumIndices; ++i) {
    console << face.mIndices[i] << " ";
  }
  console << endl;
}
