#if DIRECT3D_ENABLED
#include "d3d_common_resources.h"

namespace rei {

namespace d3d {

RenderTargetSpec::RenderTargetSpec() {
  sample_desc = DXGI_SAMPLE_DESC {1, 0};
  rt_format = DXGI_FORMAT_B8G8R8A8_UNORM;
  ds_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  ds_clear.Depth = 1.0;
  ds_clear.Stencil = 0;
}

} // namespace d3d

} // namespace rei

#endif