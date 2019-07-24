#if DIRECT3D_ENABLED
#include "d3d_common_resources.h"

namespace rei {

namespace d3d {

RenderTargetSpec::RenderTargetSpec() {
  sample_desc = DXGI_SAMPLE_DESC {1, 0};
  rt_format = ResourceFormat::B8G8R8A8_UNORM;
  ds_format = ResourceFormat::D24_UNORM_S8_UINT;
  dxgi_rt_format = DXGI_FORMAT_R8G8B8A8_UNORM;
  dxgi_ds_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  REI_ASSERT(is_right_handed);
  ds_clear.Depth = 0.0;
  ds_clear.Stencil = 0;
}

const D3D12_INPUT_ELEMENT_DESC c_input_layout[3]
  = {{
       "POSITION", 0,                  // a Name and an Index to map elements in the shader
       DXGI_FORMAT_R32G32B32A32_FLOAT, // enum member of DXGI_FORMAT; define the format of the
                                       // element
       0,                              // input slot; kind of a flexible and optional configuration
       0,                              // byte offset
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // ADVANCED, discussed later; about instancing
       0                                           // ADVANCED; also for instancing
     },
    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
      sizeof(VertexElement::pos), // skip the first 3 coordinate data
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
      sizeof(VertexElement::pos)
        + sizeof(VertexElement::color), // skip the fisrt 3 coordinnate and 4 colors ata
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

} // namespace d3d

} // namespace rei

#endif