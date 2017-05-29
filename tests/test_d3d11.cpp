// D3D11 tutorial from www.braynzarsoft.net, with modification and additional comments 

// Include and link appropriate libraries and headers //

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>  // new shader compiler from d3d11; D3DX is deprecated since windows 8
#include <DirectXMath.h>  // new math lib from d3d11 

// D3D Interfaces
IDXGISwapChain* SwapChain;
ID3D11Device* d3d11Device;
ID3D11DeviceContext* d3d11DevCon;
ID3D11RenderTargetView* renderTargetView;
ID3D11DepthStencilView* depthSentcilView;   // store the depth/stencil view 
ID3D11Texture2D* depthStencilBuffer;        // 2D texture object to store the depth/stentil buffer


// Rendering objects 
ID3D11Buffer* cubeIndexBuffer; // buffuer to hold index (for drawing primitives based on vertec
ID3D11Buffer* cubeVertBuffer;
ID3D11VertexShader* VS;
ID3D11PixelShader* PS;
ID3DBlob* VS_Buffer; // not using ID3D10Blob; new from d3dcompiler
ID3DBlob* PS_Buffer;
ID3D11InputLayout* vertLayout;

ID3D11Buffer* cbPerObjectBuffer; // buffer to store per-object tramsform matrix 


// Some math data for transform
DirectX::XMMATRIX WVP;
DirectX::XMMATRIX World;
DirectX::XMMATRIX camView;
DirectX::XMMATRIX camProjection;
DirectX::XMVECTOR camPosition;
DirectX::XMVECTOR camTarget;
DirectX::XMVECTOR camUp;

// Some math for object transformation 
DirectX::XMMATRIX cube1world;
DirectX::XMMATRIX cube2world;
DirectX::XMMATRIX Roration;
DirectX::XMMATRIX Scale;
DirectX::XMMATRIX TRanslation;
float rot = 0.01f;

// window management 
LPCTSTR WndClassName = "firstwindow";
HWND hwnd = nullptr;
HRESULT hr;

const int Width = 300;
const int Height = 300;

// Function Prototypes //

bool InitializeDirect3d11App(HINSTANCE hInstance);
void CleanUp();
bool InitScene();
void UpdateScene();
void DrawScene();

bool InitializeWindow(HINSTANCE hInstance, int ShowWnd, int width, int height, bool windowed);
int messageloop();

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


// effect constant buffer structure; 
// memory layout must match those in the cbuffer struct in Shader 
struct cbPerObject
{
  DirectX::XMMATRIX WVP;
};

cbPerObject g_cbPerObj;

// Vertex Structure and Input Data Layout
struct Vertex
{
  Vertex() {}
  Vertex(float x, float y, float z, float r, float g, float b, float a)
    : pos(x, y, z), color(r, g, b, a) {}

  DirectX::XMFLOAT3 pos;  // DirectXMath use DirectX namespace 
  DirectX::XMFLOAT4 color; 
};

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
    12, // skip the first 3 coordinate data 
    D3D11_INPUT_PER_VERTEX_DATA, 0
  },
};
UINT numElements = ARRAYSIZE(layout); // = 2


// Function Definitions // 


// Windows main function 
int WINAPI WinMain(HINSTANCE hInstance, // program instance 
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nShowCmd)
{

  if (!InitializeWindow(hInstance, nShowCmd, Width, Height, true))
  {
    MessageBox(0, "Window Initialization - Failed",
      "Error", MB_OK);
    return 0;
  }

  if (!InitializeDirect3d11App(hInstance))    //Initialize Direct3D
  {
    MessageBox(0, "Direct3D Initialization - Failed",
      "Error", MB_OK);
    return 0;
  }

  if (!InitScene())    //Initialize our scene
  {
    MessageBox(0, "Scene Initialization - Failed",
      "Error", MB_OK);
    return 0;
  }

  messageloop();

  CleanUp();

  return 0;
}


// window initialization (used in Windows main function) 
bool InitializeWindow(HINSTANCE hInstance,  // program instance 
  int ShowWnd,  // whther to show the window ? 
  int width, int height, // size of the window 
  bool windowed) // 
{
  typedef struct _WNDCLASS {  // basic window class 
    UINT cbSize;
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HANDLE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCTSTR lpszMenuName;
    LPCTSTR lpszClassName;
  } WNDCLASS;

  WNDCLASSEX wc;

  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.cbClsExtra = NULL;
  wc.cbWndExtra = NULL;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = WndClassName;
  wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

  if (!RegisterClassEx(&wc))
  {
    MessageBox(NULL, "Error registering class",
      "Error", MB_OK | MB_ICONERROR);
    return 1;
  }

  hwnd = CreateWindowEx(  // extended window class, based on the basic class 
    NULL,
    WndClassName,
    "ex window title, you can set !!!",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT,
    width, height,
    NULL,
    NULL,
    hInstance,
    NULL
  );

  if (!hwnd)
  {
    MessageBox(NULL, "Error creating window",
      "Error", MB_OK | MB_ICONERROR);
    return 1;
  }

  ShowWindow(hwnd, ShowWnd);
  UpdateWindow(hwnd);

  return true;
}


// Initialize D3D interfaces 
bool InitializeDirect3d11App(HINSTANCE hInstance)
{
  // Describe our Buffer (drawing on the window) 
  DXGI_MODE_DESC bufferDesc;
  ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));
  bufferDesc.Width = Width;
  bufferDesc.Height = Height;
  bufferDesc.RefreshRate.Numerator = 60;
  bufferDesc.RefreshRate.Denominator = 1;
  bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

  // Describe our SwapChain (multiple window buffer; usually for double-buffering) 
  DXGI_SWAP_CHAIN_DESC swapChainDesc;
  ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
  swapChainDesc.BufferDesc = bufferDesc;
  swapChainDesc.SampleDesc.Count = 1; // 1 for double buffer; 2 for triple buffer 
  swapChainDesc.SampleDesc.Quality = 0;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.BufferCount = 1;
  swapChainDesc.OutputWindow = hwnd;
  swapChainDesc.Windowed = TRUE;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  // Create our SwapChain
  hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL,
    D3D11_SDK_VERSION, &swapChainDesc, &SwapChain, &d3d11Device, NULL, &d3d11DevCon);

  // Create our BackBuffer
  ID3D11Texture2D* BackBuffer;
  hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);

  // Create our Render Target
  hr = d3d11Device->CreateRenderTargetView(BackBuffer, NULL, &renderTargetView);
  BackBuffer->Release();


  // Describe the depth/stencil buffer (it is a texture buffer)
  D3D11_TEXTURE2D_DESC depthStencilDesc;
  depthStencilDesc.Width = Width;
  depthStencilDesc.Height = Height;
  depthStencilDesc.MipLevels = 1;  // max number of mipmap level; = 0 let d3d to generate a full set of mipmap 
  depthStencilDesc.ArraySize = 1;  // number of textures 
  depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;  // 24 bit (depth) + 8 bit (stencil) 
  depthStencilDesc.SampleDesc.Count = 1;  // multisampling parameters 
  depthStencilDesc.SampleDesc.Quality = 0;
  depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
  depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL; // used as depth/stencil buffer 
  depthStencilDesc.CPUAccessFlags = 0;
  depthStencilDesc.MiscFlags = 0;

  // Create the depth/stencil buffer 
  d3d11Device->CreateTexture2D(&depthStencilDesc, NULL, &depthStencilBuffer);

  // Create the depth/stencil view object 
  d3d11Device->CreateDepthStencilView(depthStencilBuffer, NULL, &depthSentcilView);


  // Set our Render Target (with a depth/sentil ciew) 
  d3d11DevCon->OMSetRenderTargets(
    1, // number of render targets to bind 
    &renderTargetView, // render target view object pointer 
    depthSentcilView  // depth/stencil view object
  );

  return true;
}


// Release those global objects
void CleanUp()
{
  //Release the COM Objects we created
  SwapChain->Release();
  d3d11Device->Release();
  d3d11DevCon->Release();
  renderTargetView->Release();
  depthSentcilView->Release();
  depthStencilBuffer->Release();

  // Release the rendering-related object
  cubeIndexBuffer->Release();
  cubeVertBuffer->Release();
  VS->Release();
  PS->Release();
  VS_Buffer->Release();
  PS_Buffer->Release();
  vertLayout->Release();

  cbPerObjectBuffer->Release();
}


// Initialize the Scene&rendering related stuffs 
bool InitScene()
{
  // Compile Shaders from shader file
  // NOTE: new interface for D3DCompiler; different from the original tutorial  
  hr = D3DCompileFromFile(
    L"effects.hlsl",  // shader file name 
    0, // shader macros
    0, // shader includes  
    "VS", // shader entry pointer
    "vs_4_0", // shader target: shader model version or effect type 
    0, 0, // two optional flags 
    &VS_Buffer, // recieve compiled shader code 
    0 // receive optional error repot 
  );
  hr = D3DCompileFromFile(L"effects.hlsl", 0, 0, "PS", "ps_4_0", 0, 0, &PS_Buffer, 0);

  // Create the Shader Objects
  hr = d3d11Device->CreateVertexShader(
    VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(),  // specific the shader data 
    NULL, // pointer to a class linkage interface; no using now 
    &VS // receive the returned vertex shader object 
  );
  hr = d3d11Device->CreatePixelShader(PS_Buffer->GetBufferPointer(), PS_Buffer->GetBufferSize(), NULL, &PS);

  // Set Vertex and Pixel Shaders (to be used on the pipeline)
  d3d11DevCon->VSSetShader(
    VS, // compiled shader object 
    0, // set the used interface (related to the class linkage interface?); not using currently 
    0 // the number of class-instance (related to above); not using currently 
  );
  d3d11DevCon->PSSetShader(PS, 0, 0);

  // Create the vertex & index buffer // 

  // the data we will use
  Vertex v[] =
  {
    Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f),
    Vertex(-1.0f, +1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f),
    Vertex(+1.0f, +1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f),
    Vertex(+1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f),
    Vertex(-1.0f, -1.0f, +1.0f, 0.0f, 1.0f, 1.0f, 1.0f),
    Vertex(-1.0f, +1.0f, +1.0f, 1.0f, 1.0f, 1.0f, 1.0f),
    Vertex(+1.0f, +1.0f, +1.0f, 1.0f, 0.0f, 1.0f, 1.0f),
    Vertex(+1.0f, -1.0f, +1.0f, 1.0f, 0.0f, 0.0f, 1.0f),
  };
  DWORD indices[] = {
    // front face
    0, 1, 2,
    0, 2, 3,

    // back face
    4, 6, 5,
    4, 7, 6,

    // left face
    4, 5, 1,
    4, 1, 0,

    // right face
    3, 2, 6,
    3, 6, 7,

    // top face
    1, 5, 6,
    1, 6, 2,

    // bottom face
    4, 0, 3,
    4, 3, 7
  };

  // Create a buffer description for vertex data 
  D3D11_BUFFER_DESC vertexBufferDesc;
  ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
  vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;  // how the buffer will be read from and written to; use default 
  vertexBufferDesc.ByteWidth = sizeof(Vertex) * ARRAYSIZE(v);  // size of the buffer 
  vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;  // used as vertex buffer 
  vertexBufferDesc.CPUAccessFlags = 0; // how it will be used by the CPU; we don't use it 
  vertexBufferDesc.MiscFlags = 0; // extra flags; not using 
  vertexBufferDesc.StructureByteStride = NULL; // not using 

  // Create a buffer description for indices data 
  D3D11_BUFFER_DESC indexBufferDesc;
  ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
  indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;  // I guess this is for DRAM type 
  indexBufferDesc.ByteWidth = sizeof(DWORD) * ARRAYSIZE(indices);
  indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;  // different flags from vertex buffer; must be set 
  indexBufferDesc.CPUAccessFlags = 0;
  indexBufferDesc.MiscFlags = 0;
  indexBufferDesc.StructureByteStride = NULL;

  // Create the vertex buffer data object 
  D3D11_SUBRESOURCE_DATA vertexBufferData; // parameter struct ?
  ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
  vertexBufferData.pSysMem = v; // the data to be put (defined above) 
  //vertexBufferData.SysMemPitch; // width of a line in the data; used in 2D/3D texture 
  //vertexBufferData.SysMemSlicePitch; // size of a depth-level; used in 3D texture 
  hr = d3d11Device->CreateBuffer(
    &vertexBufferDesc, // buffer description 
    &vertexBufferData, // parameter set above 
    &cubeVertBuffer // receive the returned ID3D11Buffer object 
  );

  // Create the index buffer data object 
  D3D11_SUBRESOURCE_DATA indexBufferData; // parameter struct ?
  ZeroMemory(&indexBufferData, sizeof(indexBufferData));
  indexBufferData.pSysMem = indices;
  d3d11Device->CreateBuffer(&indexBufferDesc, &indexBufferData, &cubeIndexBuffer);

  // Set the vertex buffer (bind it to the Input Assembler) 
  UINT stride = sizeof(Vertex);
  UINT offset = 0;
  d3d11DevCon->IASetVertexBuffers(
    0, // the input slot we use as start 
    1, // number of buffer to bind; we bind one buffer 
    &cubeVertBuffer, // pointer to the buffer object 
    &stride, // pStrides; data size for each vertex 
    &offset // starting offset in the data 
  );

  // Set the index buffer (bind to IA)
  d3d11DevCon->IASetIndexBuffer(  // NOTE: IndexBuffer !! 
    cubeIndexBuffer, // pointer o a buffer data object; must have  D3D11_BIND_INDEX_BUFFER flag 
    DXGI_FORMAT_R32_UINT,  // data format 
    0 // UINT; starting offset in the data 
  );

  // Create the Input Layout
  d3d11Device->CreateInputLayout(
    layout, // element layout description (defined above at global scope)
    numElements, // number of elements; (also defined at global scope) 
    VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), // the shader byte code 
    &vertLayout // received the returned Input Layout  
  );

  // Set the Input Layout (bind to Input Assembler) 
  d3d11DevCon->IASetInputLayout(vertLayout);

  // Set Primitive Topology (tell InputAssemble what type of primitives we are sending) 
  // alternatives: point list, line strip, line list, triangle strip, triangle ist, 
  // primitives with adjacency (only for geometry shader)
  d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // Create the D3D Viewport (settings are used in the Rasterizer Stage) 
  D3D11_VIEWPORT viewport;
  ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
  viewport.TopLeftX = 0;  // position of 
  viewport.TopLeftY = 0;  //  the top-left corner in the window.
  viewport.Width = Width;
  viewport.Height = Height;
  viewport.MinDepth = 0.0f; // set depth range; used for converting z-values to depth  
  viewport.MaxDepth = 1.0f; // furthest value 

  // Set the Viewport (bind to the Raster Stage of he pipeline) 
  d3d11DevCon->RSSetViewports(
    1, // TODO: what are these
    &viewport
  );


  // Camera data
  camPosition = DirectX::XMVectorSet (0.0f, 2.0f, -10.0f, 0.0f); 
  camTarget = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
  camUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
  camView = DirectX::XMMatrixLookAtLH(camPosition, camTarget, camUp);  // directX Math function to created camera view transform 
  camProjection = DirectX::XMMatrixPerspectiveFovLH(0.4f * 3.14f, (float)Width / Height, 1.0f, 1000.0f);  // directX Math function 

  // Create a constant buffer for transform 
  D3D11_BUFFER_DESC cbbd;
  ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
  cbbd.Usage = D3D11_USAGE_DEFAULT;
  cbbd.ByteWidth = sizeof(cbPerObject);
  cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // NOTE: we use Constant Buffer 
  cbbd.CPUAccessFlags = 0;
  cbbd.MiscFlags = 0;
  
  d3d11Device->CreateBuffer(&cbbd, NULL, &cbPerObjectBuffer);

  return true;
}


void UpdateScene()
{
  // Keep rotating the cube 
  if ((rot += 0.0005f) > 6.28f) rot = 0.0f; // the rorate angle 

  // Reset cube world each frame //

  DirectX::XMVECTOR rotaxis = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
  Roration = DirectX::XMMatrixRotationAxis(rotaxis, rot);
  TRanslation = DirectX::XMMatrixTranslation(0.0f, 0.0f, 4.0f); // alway away from origin by 4.0 
  cube1world = TRanslation * Roration;

  Roration = DirectX::XMMatrixRotationAxis(rotaxis, -rot); // reversed direction 
  Scale = DirectX::XMMatrixScaling(1.3f, 3.0f, 1.3f); // little bit bigger than cube 1
  cube2world = Roration * Scale;
}


// Render the scene 
void DrawScene()
{
  // Clear our backbuffer
  float bgColor[4] = { 0.3f, 0.6f, 0.7f, 1.0f };
  d3d11DevCon->ClearRenderTargetView(renderTargetView, bgColor);

  // Also clear the depth/stencil view each frame 
  d3d11DevCon->ClearDepthStencilView(
    depthSentcilView,  // the view to clear 
    D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, // specify the part of the depth/stencil view to clear 
    1.0f, // clear value for depth; should set to the furtheast value (we use 1.0 as furthest) 
    0 // clear value for stencil; we actually not using stencil currently 
  );


  // Calculate the WVP matrix and send it (set to VS constant buffer in the pipeline)
  //World = DirectX::XMMatrixIdentity();
  //WVP = World * camView * camProjection;
  //g_cbPerObj.WVP = DirectX::XMMatrixTranspose(WVP);
  //d3d11DevCon->UpdateSubresource( 
  //  cbPerObjectBuffer,  // pointer to the destination buffer 
  //  0, // index for the destination (of array above)
  //  NULL,  // optional pointer to a D3D11_BOX
  //  &g_cbPerObj, // pointer to source data in memory 
  //  0,  // size of a row (used for 2D/3D buffer?)
  //  0  // size of a depth slice  (used for 3D buffer?)
  //);
  //d3d11DevCon->VSSetConstantBuffers(
  //  0,  // start slot in the constant buffer to be set 
  //  1,  // number of buffers to set (start from the slot above) 
  //  &cbPerObjectBuffer // array of constant buffer to send
  //);

  // Draw cube 1 
  WVP = cube1world * camView * camProjection;
  g_cbPerObj.WVP = DirectX::XMMatrixTranspose(WVP);  // TODO: why transpose ? 
  d3d11DevCon->UpdateSubresource(cbPerObjectBuffer, 0, NULL, &g_cbPerObj, 0, 0);
  d3d11DevCon->VSSetConstantBuffers(0, 1, &cbPerObjectBuffer);
  d3d11DevCon->DrawIndexed(36, 0, 0);

  // Draw cube 2 
  WVP = cube2world * camView * camProjection;
  g_cbPerObj.WVP = DirectX::XMMatrixTranspose(WVP);  // TODO: why transpose ? 
  d3d11DevCon->UpdateSubresource(cbPerObjectBuffer, 0, NULL, &g_cbPerObj, 0, 0);
  d3d11DevCon->VSSetConstantBuffers(0, 1, &cbPerObjectBuffer);
  d3d11DevCon->DrawIndexed(36, 0, 0);

  // Present the backbuffer to the screen
  SwapChain->Present(0, 0);   // TODO: what are there parameters 
}


int messageloop() {
  MSG msg;
  ZeroMemory(&msg, sizeof(MSG));
  while (true)
  {
    BOOL PeekMessageL(
      LPMSG lpMsg,
      HWND hWnd,
      UINT wMsgFilterMin,
      UINT wMsgFilterMax,
      UINT wRemoveMsg
    );

    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      if (msg.message == WM_QUIT)
        break;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else {
      // run game code            
      UpdateScene();
      DrawScene();
    }
  }
  return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd,
  UINT msg,
  WPARAM wParam,
  LPARAM lParam)
{
  switch (msg)
  {
  case WM_KEYDOWN:
    if (wParam == VK_ESCAPE) {
      DestroyWindow(hwnd);
    }
    return 0;

  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hwnd,
    msg,
    wParam,
    lParam);
}