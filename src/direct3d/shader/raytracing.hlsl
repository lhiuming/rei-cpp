struct PerFrameConstantBuffer {
  // NOTE: see PerFrameConstantBuffer in cpp code
  float4x4 proj_to_world;
  float4 camera_pos;
  float4 ambient_color;
  float4 light_pos;
  float4 light_color;
};
struct Vertex {
  // NOTE: see VertexElement in cpp code
  float4 pos;
  float4 color;
  float3 normal;
};

// Global
// Output
RWTexture2D<float4> render_target : register(u0);
// AS
RaytracingAccelerationStructure scene : register(t0);
// Per-frame CB
ConstantBuffer<PerFrameConstantBuffer> g_perfram_cb : register(b0);
// Per-scene meshes
ByteAddressBuffer g_indicies : register(t1);
StructuredBuffer<Vertex> g_vertices : register(t2);

typedef BuiltInTriangleIntersectionAttributes HitAttributes;

struct RayPayload {
  float4 color;
};

// REMARK: copy from dx-sample tutorial
uint3 Load3x16BitIndices(uint offsetBytes) {
  uint3 indices;

  // ByteAdressBuffer loads must be aligned at a 4 byte boundary.
  // Since we need to read three 16 bit indices: { 0, 1, 2 }
  // aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
  // we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
  // based on first index's offsetBytes being aligned at the 4 byte boundary or not:
  //  Aligned:     { 0 1 | 2 - }
  //  Not aligned: { - 0 | 1 2 }
  const uint dwordAlignedOffset = offsetBytes & ~3;
  const uint2 four16BitIndices = g_indicies.Load2(dwordAlignedOffset);

  // Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
  if (dwordAlignedOffset == offsetBytes) {
    indices.x = four16BitIndices.x & 0xffff;
    indices.y = (four16BitIndices.x >> 16) & 0xffff;
    indices.z = four16BitIndices.y & 0xffff;
  } else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
  {
    indices.x = (four16BitIndices.x >> 16) & 0xffff;
    indices.y = four16BitIndices.y & 0xffff;
    indices.z = (four16BitIndices.y >> 16) & 0xffff;
  }

  return indices;
}

[shader("raygeneration")] void raygen_shader() {
  float2 lerp_values = (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();
  float4 ndc = float4(lerp_values * 2.0f - 1.0f, 0.0f, 1.0f);
  ndc.y = -ndc.y; // default ray index counts from top-left
  float4 pixel_world_homo = mul(g_perfram_cb.proj_to_world, ndc);
  float3 pixel_world_pos = pixel_world_homo.xyz / pixel_world_homo.w;

  RayDesc ray;
  ray.Origin = g_perfram_cb.camera_pos.xyz;
  ray.Direction = normalize(pixel_world_pos - ray.Origin);
  ray.TMin = 0.001;
  ray.TMax = 10000.0;

  // debug
  // render_target[DispatchRaysIndex().xy] = float4(normalize(ray.Origin), 1.0);
  // return;

  RayPayload payload = {{0, 0, 0, 0}};
  TraceRay(scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

  render_target[DispatchRaysIndex().xy] = payload.color;
}

  [shader("closesthit")] void closest_hit_shader(inout RayPayload payload, in HitAttributes attr) {
  // hardcoded
  float3 light_pos = {2, 2, 1.5};
  float3 light_color = {0.7, 0.7, 0.7};
  float3 albedo = {0.9, 0.9, 0.9};

  // Get hitpoint normal from mesh data
  uint indexSizeInBytes = 2;
  uint indicesPerTriangle = 3;
  uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
  uint baseIndex = PrimitiveIndex() * triangleIndexStride;
  // load up 3 16 bit indices for the triangle.
  const uint3 indices = Load3x16BitIndices(baseIndex);
  // retrieve corresponding vertex normals for the triangle vertices.
  float3 n0 = g_vertices[indices[0]].normal.xyz;
  float3 n1 = g_vertices[indices[1]].normal.xyz;
  float3 n2 = g_vertices[indices[2]].normal.xyz;

  float2 bary = attr.barycentrics.xy;
  float3 n = n0 + bary.x * (n1 - n0) + bary.y * (n2 - n0);
  n = normalize(n);

  // Get hitpoint world pos
  float3 world_pos = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();

  // diffusive lighting
  float3 l = normalize(light_pos - world_pos);
  float n_dot_l = max(0.0, dot(n, l));
  float3 lighting = n_dot_l * light_color;
  float3 color = lighting * albedo;

  const float RECURSIVE_TH = 50;
  float ambient_fade = (RECURSIVE_TH - RayTCurrent()) / RECURSIVE_TH;

  if (ambient_fade > 0) {
    RayDesc ray;
    ray.Origin = world_pos;
    ray.Direction = n;
    ray.TMin = 0.001;
    ray.TMax = 10000.0 - RayTCurrent();
    TraceRay(scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);
  }

  float ambient_intensity = 0.3f * ambient_fade;

  payload.color = ambient_intensity * payload.color  + float4(color, 1.0);
}

[shader("miss")] 
void miss_shader(inout RayPayload payload) {
  float3 ayanami_blue = {129.0 / 255, 187.0 / 255, 235.0 / 255};
  float3 asuka_red = {156.0 / 255, 0, 0};
  float3 origin = WorldRayDirection();
  float mixing = clamp(1 - origin.y, 0, 2) / 2;
  mixing = pow(mixing, 3);
  payload.color = float4(lerp(ayanami_blue, asuka_red, mixing), 1.0);
}
