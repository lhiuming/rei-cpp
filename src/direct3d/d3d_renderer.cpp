// Source of d3d_renderer.h
#include "d3d_renderer.h"

#include "../console.h"

using namespace std;

namespace CEL {

// Default Constructor 
D3DRenderer::D3DRenderer()
{
  // actually have nothing todo currently 
}

// Destructor 
D3DRenderer::~D3DRenderer()
{
  // Shader objects
  VS->Release();
  PS->Release();
  VS_Buffer->Release();
  PS_Buffer->Release();

  // Pipeline states 
  //FaceRender
  //LineRender

  // Mesh buffers 
  for (auto& mb : mesh_buffers)
  {
    mb.meshIndexBuffer->Release();
    mb.meshVertBuffer->Release();
    mb.vertLayout->Release();
    mb.cbPerObjectBuffer->Release();
  }

  // scene-wide constant buffer
  //cbPerFrameBuffer

  console << "D3DRenderer is destructed." << endl;
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

  MeshBuffer mb(mesh);

  // Collect the source data 
  vector<VertexElement> vertices;
  vector<TriangleIndices> indices;
  Vec3 default_normal{ 1.0, 1.0, 1.0 };
  for (const auto& v : mesh.get_vertices())
    vertices.emplace_back(v.coord, v.color, default_normal);
  for (const auto& t : mesh.get_indices())
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
  D3D11_INPUT_ELEMENT_DESC layout[] =
  {
    { "POSITION", 0,  // a Name and an Index to map elements in the shader 
    DXGI_FORMAT_R32G32B32_FLOAT, // enum member of DXGI_FORMAT; define the format of the element
    0, // input slot; kind of a flexible and optional configuration 
    0, // byte offset 
    D3D11_INPUT_PER_VERTEX_DATA, // ADVANCED, discussed later; about instancing 
    0 // ADVANCED; also for instancing 
    },
    { "COLOR", 0,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    0,
    sizeof(VertexElement::pos), // skip the first 3 coordinate data 
    D3D11_INPUT_PER_VERTEX_DATA, 0
    },
    { "NORMAL", 0,
    DXGI_FORMAT_R32G32B32_FLOAT,
    0,
    sizeof(VertexElement::pos) + sizeof(VertexElement::color), 
    D3D11_INPUT_PER_VERTEX_DATA , 0
    }
  };
  d3d11Device->CreateInputLayout(
    layout, // element layout description (defined above at global scope)
    ARRAYSIZE(layout), // number of elements; (also defined at global scope) 
    VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), // the shader 
    &(mb.vertLayout) // received the returned Input Layout  
  );

  // Create a perObject Buffer
  D3D11_BUFFER_DESC cbbd;
  ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
  cbbd.Usage = D3D11_USAGE_DEFAULT;
  cbbd.ByteWidth = sizeof(cbPerObject);
  cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  d3d11Device->CreateBuffer(&cbbd, NULL, &(mb.cbPerObjectBuffer));

}


// Render request //

// Change buffer size 
void D3DRenderer::set_buffer_size(BufferSize width, BufferSize height)
{
  // Update storage 
  this->width = width;
  this->height = height;

  // Update the view port (used in the Rasterizer Stage) 
  D3D11_VIEWPORT viewport;
  ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
  viewport.TopLeftX = 0;  // position of 
  viewport.TopLeftY = 0;  //  the top-left corner in the window.
  viewport.Width = static_cast<float>(width);
  viewport.Height = static_cast<float>(height);
  viewport.MinDepth = 0.0f; // set depth range; used for converting z-values to depth  
  viewport.MaxDepth = 1.0f; // furthest value 

  // Set the Viewport (bind to the Raster Stage of he pipeline) 
  d3d11DevCon->RSSetViewports(1, &viewport);
}

// Do rendering 
void D3DRenderer::render() {

  // TODO : update perFramebuffers : light, 

  // render all buffered meshes 
  render_meshes();
}

// Render the meshes 
void D3DRenderer::render_meshes() {
  // TODO 

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
    //TODO : incorporation camera 
    cbPerObject cbMesh;
    d3d11DevCon->UpdateSubresource(mb.cbPerObjectBuffer, 0, NULL, &cbMesh, 0, 0);
    d3d11DevCon->VSSetConstantBuffers(0, 1, &(mb.cbPerObjectBuffer));

    // 5. DrawIndexed. 
    d3d11DevCon->DrawIndexed(mb.indices_num(), 0, 0);

    // Optional : set render state 
    //d3d11DevCon->RSSetState(SolidRender);
  }
}

} // namespace CEL