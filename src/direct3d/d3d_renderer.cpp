// Source of d3d_renderer.h
#include "d3d_renderer.h"

#include "../console.h"

using namespace std;

namespace CEL {

// Default Constructor 
D3DRenderer::D3DRenderer()
{
  // TODO : actually have nothing todo currently 
}

// Destructor 
D3DRenderer::~D3DRenderer()
{
  // TODO : check all private objects before running 

  // Shader objects
  VS->Release();
  PS->Release();
  VS_Buffer->Release();
  PS_Buffer->Release();

  // Mesh buffers 
  // TODO 

  console << "D3DRenderer is destructed." << endl;
  console << "Mesh buffers are not released." << endl;
}


// Configurations //

// compiler shaders after set_d3d_interface 
void D3DRenderer::compile_shader()
{
  // TODO : do error reporting 

  HRESULT hr;

  // Prepare Shaders //

  // Compiling 
  hr = D3DCompileFromFile(
    L"effects.hlsl",  // shader file name 
    0, // shader macros
    0, // shader includes  
    "VS", // shader entry pointer
    "vs_4_0", // shader target: shader model version or effect type 
    0, 0, // two optional flags 
    &(this->VS_Buffer), // recieve the compiled shader bytecode 
    0 // receive optional error repot 
  );
  hr = D3DCompileFromFile(
    L"effects.hlsl",
    0, 0,
    "PS", "ps_4_0",
    0, 0,
    &(this->PS_Buffer),
    0
  );

  // Create the Shader Objects
  hr = this->d3d11Device->CreateVertexShader(
    this->VS_Buffer->GetBufferPointer(),
    this->VS_Buffer->GetBufferSize(),  // specific the shader data 
    NULL, // pointer to a class linkage interface; no using now 
    &(this->VS) // receive the returned vertex shader object 
  );
  hr = this->d3d11Device->CreatePixelShader(
    PS_Buffer->GetBufferPointer(),
    PS_Buffer->GetBufferSize(),
    NULL,
    &(this->PS)
  );

  // Set the Shaders (bind to the pipeline) 
  this->d3d11DevCon->VSSetShader(
    this->VS, // compiled shader object 
    0, // set the used interface; not using currently 
    0 // the number of class-instance (related to above); not using currently 
  );
  this->d3d11DevCon->PSSetShader(
    this->PS,
    0,
    0
  );

}

// set up view

// set and initialize internal scenes 
void D3DRenderer::set_scene(shared_ptr<const Scene> scene)
{
  Renderer::set_scene(scene);

  // TODO : create perFrameObject buffer : light 

  // Reset the shaders (why?) 

  // TODO : turn the scene into internal format, ready for D3D rendering 
  for (auto& modelIns : scene->get_models())
  {
    add_mesh_buffer(modelIns);
  }
}

void D3DRenderer::add_mesh_buffer(const ModelInstance& modelIns)
{
  const Mat4& model_trans = modelIns.transform;
  const Mesh& mesh = dynamic_cast<Mesh&>(*(modelIns.pmodel));

  MeshBuffer mb;

  // Collect the source data 
  vector<VertexElement> vertices;
  vector<TriangleIndices> indices;
  Vec3 default_normal{ 1.0, 1.0, 1.0 };
  for (const auto& v : mesh.get_vertices())
    vertices.emplace_back(v.coord, v.color, default_normal);
  for (const auto& t : mesh.get_triangles())
    indices.emplace_back(t.a, t.b, t.c);

  // Create a buffer description for vertex data 
  D3D11_BUFFER_DESC vertexBufferDesc;
  ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
  vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;  
  vertexBufferDesc.ByteWidth = vertices.size() * sizeof(vertices[0]);  
  vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;  

  // Create a buffer description for indices data 
  D3D11_BUFFER_DESC indexBufferDesc;
  ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
  indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;  
  indexBufferDesc.ByteWidth = indices.size() * sizeof(indices[0]);
  indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER; 

  // Create the vertex buffer data object 
  D3D11_SUBRESOURCE_DATA vertexBufferData;
  ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
  vertexBufferData.pSysMem = &(vertices[0]); 
  d3d11Device->CreateBuffer(
    &vertexBufferDesc, // buffer description 
    &vertexBufferData, // parameter set above 
    &(mb.meshVertBuffer) // receive the returned ID3D11Buffer object 
  );

  // Create the index buffer data object 
  D3D11_SUBRESOURCE_DATA indexBufferData; // parameter struct ?
  ZeroMemory(&indexBufferData, sizeof(indexBufferData));
  indexBufferData.pSysMem = &(indices[0]);
  d3d11Device->CreateBuffer(
    &indexBufferDesc, 
    &indexBufferData, 
    &(mb.meshIndexBuffer)
  );

  // Create a Input layout for the mesh 

  // Create a perObject Buffer
}


// Render request //

// Change buffer size 
void D3DRenderer::set_buffer_size(BufferSize width, BufferSize height)
{
  // TODO 
  this->width = width;
  this->height = height;

  // TODO : set the view port 
  // Create the D3D Viewport (settings are used in the Rasterizer Stage) 
  D3D11_VIEWPORT viewport;
  ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
  viewport.TopLeftX = 0;  // position of 
  viewport.TopLeftY = 0;  //  the top-left corner in the window.
  viewport.Width = width;
  viewport.Height = height;
  viewport.MinDepth = 0.0f; // set depth range; used for converting z-values to depth  
  viewport.MaxDepth = 1.0f; // furthest value 

  // Set the Viewport (bind to the Raster Stage of he pipeline) 
  this->d3d11DevCon->RSSetViewports(
    1, // TODO: what are these
    &viewport
  );
}

// Do rendering 
void D3DRenderer::render() {
  // TODO 

  // TODO : clearRenderTargetView
  // TODO : clear depthBuffer 

  // TODO : update perFramebuffers : light, 

  // render_meshes();
}

// Render the meshes 
void D3DRenderer::render_meshes() {
  // TODO 

  // overall : IASetPrimitiveTopology

  // for each meshBuffer
  // 1. IASetVertexBuffers
  // 2. IASetIndexBuffer
  // 3. IASetInputLayout
  // 4. update perObject Buffer : WVP, World 
  // 5. DrawIndexed. 

  // Optional : set render state 

}

} // namespace CEL