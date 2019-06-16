#if DIRECT3D_ENABLED

#include "../console.h"
#include "d3d_renderer.h"

#include "d3d_device_resources.h"


#include <d3dx12.h>

using std::vector;
using std::shared_ptr;
using std::runtime_error;

namespace rei {

D3DDeviceResources::D3DDeviceResources(HINSTANCE h_inst) : hinstance(hinstance) {
  HRESULT hr;
  UINT creationFlags = 0;
  #ifndef NDEBUG
  creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
  #endif
  hr = D3D11CreateDevice(nullptr, // default vieo card
    D3D_DRIVER_TYPE_HARDWARE,     // driver type to create
    NULL,                 // handle to software rasterizer DLL; used when above is *_SOFTWARE
    creationFlags,        // enbling debug layers (see MSDN)
    NULL,                 // optional; pointer to array of feature levels
    NULL,                 // optional; number of candidate feature levels (in array above)
    D3D11_SDK_VERSION,    // fixed for d3d11
    &(this->d3d11Device), // receive the returned d3d device pointer
    NULL,                 // receive the returned featured level that actually used
    &(this->d3d11DevCon)  // receive the return d3d device context pointer
  );
  if (FAILED(hr)) throw runtime_error("Device creation FAILED");
}

// Destructor
D3DDeviceResources::~D3DDeviceResources() {
  // Release D3D interfaces
  d3d11Device->Release();
  d3d11DevCon->Release();

 // Shader objects
  VS->Release();
  PS->Release();
  VS_Buffer->Release();
  PS_Buffer->Release();

  // Pipeline states
  FaceRender->Release();
  // LineRender

  // Shared Rendering objects
  vertElementLayout->Release();
  cbPerFrameBuffer->Release();

  // Default cube stuffs
  cubeIndexBuffer->Release();
  cubeVertBuffer->Release();
  cubeConstBuffer->Release();

  // Mesh buffers
  for (auto& mb : mesh_buffers) {
    mb.meshIndexBuffer->Release();
    mb.meshVertBuffer->Release();
    mb.meshConstBuffer->Release();
    console << "A MeshBuffer is destructed." << endl;
  }
  if (mesh_buffers.empty()) console << "No MeshBuffer to be destructed" << endl;

}

// INIT: compile the default shaders
void D3DDeviceResources::compile_shader() {
  // Compile shader from file "direct3d/shader.hlsl", and bind to
  // vertex- and pixel-stage.

  HRESULT hr; // error reporting

  // Compiling
  hr = D3DCompileFromFile(L"CoreData/shader/shader.hlsl", // shader file name
    0,                                                    // shader macros
    0,                                                    // shader includes
    "VS",                                                 // shader entry pointer
    "vs_4_0",           // shader target: shader model version or effect type
    0, 0,               // two optional flags
    &(this->VS_Buffer), // recieve the compiled shader bytecode
    0                   // receive optional error repot
  );
  if (FAILED(hr)) throw runtime_error("Vertex Shader compile FAILED");
  hr = D3DCompileFromFile(
    L"CoreData/shader/shader.hlsl", 0, 0, "PS", "ps_4_0", 0, 0, &(this->PS_Buffer), 0);
  if (FAILED(hr)) throw runtime_error("Pixel Shader compile FAIED");

  // Create the Shader Objects
  hr = this->d3d11Device->CreateVertexShader(this->VS_Buffer->GetBufferPointer(),
    this->VS_Buffer->GetBufferSize(), // specific the shader data
    NULL,                             // pointer to a class linkage interface; no using now
    &(this->VS)                       // receive the returned vertex shader object
  );
  if (FAILED(hr)) throw runtime_error("Vertex Shader createion FAILED");
  hr = this->d3d11Device->CreatePixelShader(
    this->PS_Buffer->GetBufferPointer(), this->PS_Buffer->GetBufferSize(), NULL, &(this->PS));
  if (FAILED(hr)) throw runtime_error("Pixel Shader createion FAILED");

  // Set the Shaders (bind to the pipeline)
  this->d3d11DevCon->VSSetShader(this->VS,
    0, // set the used interface; not using currently
    0  // the number of class-instance (related to above); not using currently
  );
  this->d3d11DevCon->PSSetShader(this->PS, 0, 0);
}

// INIT: setup render states
void D3DDeviceResources::create_render_states() {
  HRESULT hr; // for error reporting

  // FaceRender
  D3D11_RASTERIZER_DESC FRdesc;
  ZeroMemory(&FRdesc, sizeof(D3D11_RASTERIZER_DESC));
  FRdesc.FillMode = D3D11_FILL_SOLID;
  FRdesc.CullMode = D3D11_CULL_NONE;   // TOOD: set to Back after debug
  FRdesc.FrontCounterClockwise = true; // FIXME : check vertex order
  hr = this->d3d11Device->CreateRasterizerState(&FRdesc, &(this->FaceRender));
  if (FAILED(hr)) throw runtime_error("FaceRender State creation FAILED");

  // Optional: line Render
}






// Set and initialize internal scenes
void D3DDeviceResources::set_scene(shared_ptr<const Scene> scene) {

  HRESULT hr; // for error reporting

  // Initialize some scene-rendering objects //

  // Initialize a defautl Light data
  g_light.dir = DirectX::XMFLOAT3(0.25f, 0.5f, 0.0f);
  g_light.ambient = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
  g_light.diffuse = DirectX::XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);

  // Specify the per-frame constant-buffer
  D3D11_BUFFER_DESC cbbd;
  ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC)); // reuse the DESC struct above
  cbbd.Usage = D3D11_USAGE_DEFAULT;
  cbbd.ByteWidth = sizeof(cbPerFrame);
  cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // NOTE: we use Constant Buffer
  cbbd.CPUAccessFlags = 0;
  cbbd.MiscFlags = 0;
  hr = d3d11Device->CreateBuffer(&cbbd, NULL, &cbPerFrameBuffer);
  if (FAILED(hr)) throw runtime_error("Per-frame Buufer creation FAILED");

  // Create the Input Layout for the vertex element
  hr = d3d11Device->CreateInputLayout(
    vertex_element_layout_desc, // element layout description (defined above at global scope)
    ARRAYSIZE(vertex_element_layout_desc), // number of elements; (also defined at global scope)
    VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), // the shader byte code
    &vertElementLayout                                         // received the returned Input Layout
  );

  // Set the Input Layout (bind to Input Assembler)
  d3d11DevCon->IASetInputLayout(vertElementLayout);

  // Initialze object-specific stuffs //

  // Initialize the default cube scene
  this->initialize_default_scene();

  // Initialize MeshBuffer for all mesh in the scene
  for (const auto& modelIns : scene->get_models()) {
    add_mesh_buffer(modelIns);
  }
}






void D3DDeviceResources::initialize_default_scene() {
  // Create the vertex & index buffer for a cetralized cube //

  HRESULT hr;

  // the data for the cube
  VertexElement v[] = {
    //     Position               Color                     Normal
    VertexElement(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f),
    VertexElement(-1.0f, +1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, +1.0f, -1.0f),
    VertexElement(+1.0f, +1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, +1.0f, +1.0f, -1.0f),
    VertexElement(+1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, +1.0f, -1.0f, -1.0f),
    VertexElement(-1.0f, -1.0f, +1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, +1.0f),
    VertexElement(-1.0f, +1.0f, +1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, +1.0f, +1.0f),
    VertexElement(+1.0f, +1.0f, +1.0f, 1.0f, 1.0f, 1.0f, 1.0f, +1.0f, +1.0f, +1.0f),
    VertexElement(+1.0f, -1.0f, +1.0f, 1.0f, 1.0f, 1.0f, 1.0f, +1.0f, -1.0f, +1.0f),
  };
  DWORD indices[] = {// front face
    0, 1, 2, 0, 2, 3,

    // back face
    4, 6, 5, 4, 7, 6,

    // left face
    4, 5, 1, 4, 1, 0,

    // right face
    3, 2, 6, 3, 6, 7,

    // top face
    1, 5, 6, 1, 6, 2,

    // bottom face
    4, 0, 3, 4, 3, 7};

  // Create the vertex buffer data object
  D3D11_BUFFER_DESC vertexBufferDesc;
  ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
  vertexBufferDesc.Usage
    = D3D11_USAGE_DEFAULT; // how the buffer will be read from and written to; use default
  vertexBufferDesc.ByteWidth = sizeof(VertexElement) * ARRAYSIZE(v); // size of the buffer
  vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;             // used as vertex buffer
  vertexBufferDesc.CPUAccessFlags = 0;                               //  we don't use it
  vertexBufferDesc.MiscFlags = 0;                                    // extra flags; not using
  vertexBufferDesc.StructureByteStride = NULL;                       // not using
  D3D11_SUBRESOURCE_DATA vertexBufferData;                           // parameter struct ?
  ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
  vertexBufferData.pSysMem = v;
  hr = d3d11Device->CreateBuffer(&vertexBufferDesc, // buffer description
    &vertexBufferData,                              // parameter set above
    &(this->cubeVertBuffer)                         // receive the returned ID3D11Buffer object
  );
  if (FAILED(hr)) throw runtime_error("Create cube vertex buffer FAILED");

  // Create the index buffer data object
  D3D11_BUFFER_DESC indexBufferDesc;
  ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
  indexBufferDesc.Usage = D3D11_USAGE_DEFAULT; // I guess this is for DRAM type
  indexBufferDesc.ByteWidth = sizeof(DWORD) * ARRAYSIZE(indices);
  indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER; // index !
  indexBufferDesc.CPUAccessFlags = 0;
  indexBufferDesc.MiscFlags = 0;
  indexBufferDesc.StructureByteStride = NULL;
  D3D11_SUBRESOURCE_DATA indexBufferData;
  ZeroMemory(&indexBufferData, sizeof(indexBufferData));
  indexBufferData.pSysMem = indices;
  hr = d3d11Device->CreateBuffer(&indexBufferDesc, &indexBufferData, &(this->cubeIndexBuffer));
  if (FAILED(hr)) throw runtime_error("Create cube index buffer FAILED");

  // Create a constant-buffer for the cube
  D3D11_BUFFER_DESC cbbd;
  ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
  cbbd.Usage = D3D11_USAGE_DEFAULT;
  cbbd.ByteWidth = sizeof(cbPerObject);
  cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // NOTE: we use Constant Buffer
  hr = d3d11Device->CreateBuffer(&cbbd, NULL, &cubeConstBuffer);
  if (FAILED(hr)) throw runtime_error("Cube Const Buffer creation FAILED");
}




// Helper to set_scene
void D3DDeviceResources::add_mesh_buffer(const ModelInstance& modelIns) {
  const Mat4& model_stran = modelIns.transform;
  const Mesh& mesh = dynamic_cast<Mesh&>(*modelIns.pmodel);

  // Take control of the creation of MeshBuffer

  HRESULT hr; // for error reporting

  mesh_buffers.emplace_back(mesh);
  MeshBuffer& mb = mesh_buffers.back();

  // Collect the source data
  vector<VertexElement> vertices;
  vector<DWORD> indices;
  Vec3 default_normal {1.0, 1.0, 1.0};
  for (const auto& v : mesh.get_vertices())
    vertices.emplace_back(v.coord, v.color, default_normal);
  for (const auto& t : mesh.get_triangles()) {
    indices.push_back(t.a);
    indices.push_back(t.b);
    indices.push_back(t.c);
  }

  // Create the vertex buffer data object
  D3D11_BUFFER_DESC vertexBufferDesc;
  ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
  vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
  vertexBufferDesc.ByteWidth = vertices.size() * sizeof(vertices[0]);
  vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  console << "vertex buffer byte width is " << vertexBufferDesc.ByteWidth << endl;
  D3D11_SUBRESOURCE_DATA vertexBufferData;
  ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
  vertexBufferData.pSysMem = &(vertices[0]);
  hr = d3d11Device->CreateBuffer(&vertexBufferDesc, // buffer description
    &vertexBufferData,                              // parameter set above
    &(mb.meshVertBuffer)                            // receive the returned ID3D11Buffer object
  );
  if (FAILED(hr)) throw runtime_error("Mesh Vertex Buffer creation FAILED");

  // Create the index buffer data object
  D3D11_BUFFER_DESC indexBufferDesc;
  ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
  indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
  indexBufferDesc.ByteWidth = indices.size() * sizeof(indices[0]);
  indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
  console << "index buffer byte width is " << indexBufferDesc.ByteWidth << endl;
  D3D11_SUBRESOURCE_DATA indexBufferData; // parameter struct ?
  ZeroMemory(&indexBufferData, sizeof(indexBufferData));
  indexBufferData.pSysMem = &(indices[0]);
  hr = d3d11Device->CreateBuffer(&indexBufferDesc, &indexBufferData, &(mb.meshIndexBuffer));
  if (FAILED(hr)) throw runtime_error("Mesh Index Buffer creation FAILED");

  // Create a constant-buffer for the cube
  D3D11_BUFFER_DESC cbbd;
  ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
  cbbd.Usage = D3D11_USAGE_DEFAULT;
  cbbd.ByteWidth = sizeof(cbPerObject);
  cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // NOTE: we use Constant Buffer
  hr = d3d11Device->CreateBuffer(&cbbd, NULL, &(mb.meshConstBuffer));
  if (FAILED(hr)) throw runtime_error("Cube Const Buffer creation FAILED");
}


}

#endif
