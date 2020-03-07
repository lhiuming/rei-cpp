#ifndef REI_D3D_RESOURCE_TYPES
#define REI_D3D_RESOURCE_TYPES

#define NOMINMAX
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <windows.h>
#include <wrl.h>
#undef NOMINMAX

#include "../variant_utils.h"

namespace rei::d3d {

using Microsoft::WRL::ComPtr;

struct CommittedResource {
  ComPtr<ID3D12Resource> ptr;
  UINT64 bytesize;
  bool is_system_memory;
  CommittedResource() : ptr(NULL), bytesize(-1), is_system_memory(false) {}
  CommittedResource(ComPtr<ID3D12Resource>&& res_ptr, UINT64 bsize, bool is_sysmem) : ptr(res_ptr), bytesize(bsize), is_system_memory(is_sysmem) {}
  CommittedResource(const ComPtr<ID3D12Resource>& res_ptr, UINT64 bsize, bool is_sysmem) : ptr(res_ptr), bytesize(bsize), is_system_memory(is_sysmem) {}
};

} // namespace rei::d3d


#endif