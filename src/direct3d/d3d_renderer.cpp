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



// Render request //

// Change buffer size 
void D3DRenderer::set_buffer_size(BufferSize width, BufferSize height)
{
  // TODO 
  this->width = width;
  this->height = height;
}

// Do rendering 
void D3DRenderer::render() {
  // TODO 
}

} // namespace CEL