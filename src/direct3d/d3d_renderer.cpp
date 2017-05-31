// Source of d3d_renderer.h
#include "d3d_renderer.h"

#include "../console.h"

using namespace std;

namespace CEL {

// Default Constructor 
D3DRenderer::D3DRenderer()
{ }

// Destructor 
D3DRenderer::~D3DRenderer()
{
  // Shader objects
  VS->Release();
  PS->Release();
  VS_Buffer->Release();
  PS_Buffer->Release();

  // Pipeline states 
  FaceRender->Release();
  //LineRender

  // Mesh buffers 
  for (auto& mb : mesh_buffers)
  {
    mb.meshIndexBuffer->Release();
    mb.meshVertBuffer->Release();
    mb.vertLayout->Release();
    mb.cbPerObjectBuffer->Release();
    console << "A MeshBuffer is destructed." << endl;
  }
  if (mesh_buffers.empty()) console << "No MeshBuffer to be destructed" << endl;

  // scene-wide constant buffer
  cbPerFrameBuffer->Release();

  console << "D3DRenderer is destructed." << endl;
}


// Configurations //

// INIT: compiler shaders after set_d3d_interface 
void D3DRenderer::compile_shader()
{
  HRESULT hr; // for error reporting 

  // Compiling 
  hr = D3DCompileFromFile(
    L"direct3d/shader.hlsl",  // shader file name 
    0, // shader macros
    0, // shader includes  
    "VS", // shader entry pointer
    "vs_4_0", // shader target: shader model version or effect type 
    0, 0, // two optional flags 
    &(this->VS_Buffer), // recieve the compiled shader bytecode 
    0 // receive optional error repot 
  );
  if (FAILED(hr)) throw runtime_error("Vertex Shader compile FAILED");
  hr = D3DCompileFromFile(
    L"direct3d/shader.hlsl",
    0, 0,
    "PS", "ps_4_0",
    0, 0,
    &(this->PS_Buffer),
    0
  );
  if (FAILED(hr)) throw runtime_error("Pixel Shader compile FAIED");

  // Create the Shader Objects
  hr = this->d3d11Device->CreateVertexShader(
    this->VS_Buffer->GetBufferPointer(),
    this->VS_Buffer->GetBufferSize(),  // specific the shader data 
    NULL, // pointer to a class linkage interface; no using now 
    &(this->VS) // receive the returned vertex shader object 
  );
  if (FAILED(hr)) throw runtime_error("Vertex Shader createion FAILED");
  hr = this->d3d11Device->CreatePixelShader(
    this->PS_Buffer->GetBufferPointer(),
    this->PS_Buffer->GetBufferSize(),
    NULL,
    &(this->PS)
  );
  if (FAILED(hr)) throw runtime_error("Pixel Shader createion FAILED");

  // Set the Shaders (bind to the pipeline) 
  this->d3d11DevCon->VSSetShader(this->VS,
    0, // set the used interface; not using currently 
    0 // the number of class-instance (related to above); not using currently 
  );
  this->d3d11DevCon->PSSetShader(this->PS, 0, 0);

}

// INIT: setup render states
void D3DRenderer::create_render_states()
{
  HRESULT hr; // for error reporting 

  // FaceRender 
  D3D11_RASTERIZER_DESC FRdesc;
  ZeroMemory(&FRdesc, sizeof(D3D11_RASTERIZER_DESC));
  FRdesc.FillMode = D3D11_FILL_SOLID;
  FRdesc.CullMode = D3D11_CULL_NONE; // TOOD: set to Back after debug 
  FRdesc.FrontCounterClockwise = true; // FIXME : which way camera looks?
  hr = this->d3d11Device->CreateRasterizerState(&FRdesc, &(this->FaceRender));
  if (FAILED(hr)) throw runtime_error("FaceRender State creation FAILED");

  // Optional: line Render 

}

// set and initialize internal scenes 
void D3DRenderer::set_scene(shared_ptr<const Scene> scene)
{
  Renderer::set_scene(scene);

  // create perFrameObject buffer : light 
  // FIXME : load light from scene 
  // FIXME : release the old object if it is outdated
  D3D11_BUFFER_DESC cbDesc;
  ZeroMemory(&cbDesc, sizeof(D3D11_BUFFER_DESC)); // reuse the DESC struct above
  cbDesc.Usage = D3D11_USAGE_DEFAULT;
  cbDesc.ByteWidth = sizeof(cbPerFrame);
  cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // NOTE: we use Constant Buffer 
  d3d11Device->CreateBuffer(&cbDesc, NULL, &(this->cbPerFrameBuffer));

  // turn the scene into internal format, ready for D3D rendering 
  for (auto& modelIns : scene->get_models())
  {
    add_mesh_buffer(modelIns);  
  }

}

void D3DRenderer::add_mesh_buffer(const ModelInstance& modelIns)
{
  const Mat4& model_trans = modelIns.transform;
  const Mesh& mesh = dynamic_cast<Mesh&>(*(modelIns.pmodel));

  // Take control of the creation of MeshBuffer 

  HRESULT hr; // for error reporting 

  mesh_buffers.emplace_back(mesh);
  MeshBuffer& mb = mesh_buffers.back();

  // Collect the source data 
  vector<VertexElement> vertices;
  vector<DWORD> indices;
  Vec3 default_normal{ 1.0, 1.0, 1.0 };
  for (const auto& v : mesh.get_vertices())
    vertices.emplace_back(v.coord, v.color, default_normal);
  for (const auto& t : mesh.get_indices())
  {
    indices.push_back(t.a);
    indices.push_back(t.b);
    indices.push_back(t.c);
  }

  // Make a buffer description for vertex data 
  D3D11_BUFFER_DESC vertexBufferDesc;
  ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
  vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;  
  vertexBufferDesc.ByteWidth = vertices.size() * sizeof(vertices[0]);  
  vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;  
  console << "vertex buffer byte width is " << vertexBufferDesc.ByteWidth << endl;
 
  // Create the vertex buffer data object 
  D3D11_SUBRESOURCE_DATA vertexBufferData;
  ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
  vertexBufferData.pSysMem = &(vertices[0]); 
  hr = d3d11Device->CreateBuffer(
    &vertexBufferDesc, // buffer description 
    &vertexBufferData, // parameter set above 
    &(mb.meshVertBuffer) // receive the returned ID3D11Buffer object 
  );
  if (FAILED(hr)) throw runtime_error("Vertex Buffer creation FAILED");

  // Make a buffer description for indices data 
  D3D11_BUFFER_DESC indexBufferDesc;
  ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
  indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
  indexBufferDesc.ByteWidth = indices.size() * sizeof(indices[0]);
  indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
  console << "index buffer byte width is " << indexBufferDesc.ByteWidth << endl;

  // Create the index buffer data object 
  D3D11_SUBRESOURCE_DATA indexBufferData; // parameter struct ?
  ZeroMemory(&indexBufferData, sizeof(indexBufferData));
  indexBufferData.pSysMem = &(indices[0]);
  hr = d3d11Device->CreateBuffer(
    &indexBufferDesc, 
    &indexBufferData, 
    &(mb.meshIndexBuffer)
  );
  if (FAILED(hr)) throw runtime_error("Index Buffer creation FAILED");

  // Create a Input layout for the mesh 
  D3D11_INPUT_ELEMENT_DESC layout[] =
  {
    { "POSITION", 0,  // a Name and an Index to map elements in the shader 
    DXGI_FORMAT_R32G32B32A32_FLOAT, // define the format of the element
    0, // input slot; kind of a flexible and optional configuration 
    0, // byte offset 
    D3D11_INPUT_PER_VERTEX_DATA, // ADVANCED, discussed later; about instancing 
    0 // ADVANCED; also for instancing 
    },
    { "COLOR", 0,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    0,
    16, // skip the first 4 position data 
    D3D11_INPUT_PER_VERTEX_DATA, 0
    },
    { "NORMAL", 0,
    DXGI_FORMAT_R32G32B32_FLOAT,
    0,
    32, 
    D3D11_INPUT_PER_VERTEX_DATA , 0
    }
  };
  hr = d3d11Device->CreateInputLayout(
    layout, // element layout description 
    ARRAYSIZE(layout), // number of elements
    VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), // the shader 
    &(mb.vertLayout) // received the returned Input Layout  
  );
  if (FAILED(hr)) throw runtime_error("Input Layout creation FAILED");

  // Create a perObject Buffer
  D3D11_BUFFER_DESC cbbd;
  ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
  cbbd.Usage = D3D11_USAGE_DEFAULT;
  cbbd.ByteWidth = sizeof(cbPerObject);
  cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  hr = d3d11Device->CreateBuffer(&cbbd, NULL, &(mb.cbPerObjectBuffer));
  if (FAILED(hr)) throw runtime_error("Contant Buffer per-mesh creation FAILED");

}


// Render request //

// Change buffer size 
void D3DRenderer::set_buffer_size(BufferSize width, BufferSize height)
{
  // Update storage 
  this->width = width;
  this->height = height;
}

// Do rendering 
void D3DRenderer::render() {

  // Optional : update perFramebuffers : light, 
  cbPerFrame cbPerF;
  d3d11DevCon->UpdateSubresource(cbPerFrameBuffer, 0, NULL, &cbPerF, 0, 0);
  d3d11DevCon->PSSetConstantBuffers(0, 1, &(this->cbPerFrameBuffer)); 

  // Reset the shaders : why? FIXME
  this->d3d11DevCon->VSSetShader(this->VS, 0, 0);
  this->d3d11DevCon->PSSetShader(this->PS, 0, 0);

  // render all buffered meshes 
  render_meshes();

}

// Render the meshes 
void D3DRenderer::render_meshes() {
  // Use TRIANGLELIST mode 
  d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // for each meshBuffer
  for (auto& mb : mesh_buffers)
  {

    // 1. set vertex buffer 
    UINT stride = sizeof(VertexElement);
    UINT offset = 0;
    d3d11DevCon->IASetVertexBuffers(
      0, // the input slot we use as start 
      1, // number of buffer to bind; we bind one buffer
      &(mb.meshVertBuffer), // pointer to the buffer object 
      &stride, // pStrides; data size for each vertex 
      &offset // starting offset in the data 
    );

    // 2. set indices buffer
    d3d11DevCon->IASetIndexBuffer(
      mb.meshIndexBuffer, // pointer to a buffer data object      
      DXGI_FORMAT_R32_UINT,  // data format 
      0 // unsigned int; starting offset in the data 
    );

    // 3. set per-vertex input layout
    d3d11DevCon->IASetInputLayout(mb.vertLayout);

    // 4. update and set perObject Buffer : WVP, World 
    // TODO add model transform before w2n
    cbPerObject cbMesh(camera->get_w2n());
    d3d11DevCon->UpdateSubresource(mb.cbPerObjectBuffer, 0, NULL, &cbMesh, 0, 0);
    d3d11DevCon->VSSetConstantBuffers(0, 1, &(mb.cbPerObjectBuffer));

    // set render state 
    d3d11DevCon->RSSetState(FaceRender);

    // 5. DrawIndexed. 
    d3d11DevCon->DrawIndexed(mb.indices_num(), 0, 0);


  }
}

} // namespace CEL