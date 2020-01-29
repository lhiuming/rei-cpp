#include "d3d_common_resources.h"

#include <typeinfo>

#include "../renderer.h"

namespace rei {

namespace d3d {

RenderTargetSpec::RenderTargetSpec() {
  sample_desc = DXGI_SAMPLE_DESC {1, 0};
  rt_format = ResourceFormat::R8G8B8A8Unorm;
  ds_format = ResourceFormat::D24Unorm_S8Uint;
  dxgi_rt_format = DXGI_FORMAT_R8G8B8A8_UNORM;
  dxgi_ds_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  REI_ASSERT(is_right_handed);
  ds_clear.Depth = 0.0;
  ds_clear.Stencil = 0;
}

ID3D12Resource* BufferData::get_res() {
  return res.match( //
    [](const TextureBuffer& tex) { return tex.buffer.Get(); },
    [](const BlasBuffer& blas) { return blas.res.ptr.Get(); },
    [](const auto& b) {
      REI_NOT_IMPLEMENT();
      const std::type_info& i = typeid(b);
      warning(i.name());
      return (ID3D12Resource*)NULL;
    });
}


} // namespace d3d

} // namespace rei
