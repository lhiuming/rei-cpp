#include "halton.hlsl"

#define PI 3.141592653589793238462643383279502884197169399375105820974f

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
RWTexture2D<float4> render_target : register(u0, space0);
// AS
RaytracingAccelerationStructure scene : register(t0, space0);
// Per-frame CB
ConstantBuffer<PerFrameConstantBuffer> g_perfram_cb : register(b0, space0);

// Local: Hit Group //

//  mesh data
ByteAddressBuffer g_indicies : register(t0, space1);
StructuredBuffer<Vertex> g_vertices : register(t1, space1);

// Tracing data //

typedef BuiltInTriangleIntersectionAttributes HitAttributes;

struct RayPayload {
  float4 color;
  int depth;
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

  RayPayload payload = {{0, 0, 0, 0}, 0};
  TraceRay(scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 0, 0, ray, payload);

  render_target[DispatchRaysIndex().xy] = payload.color;
}

// TODO add tangent to vertex data
// TODO importance sampling
float3 sample_lambertian_brdf(float3 normal, float rnd0, float rnd1, out float probability) {
  // compute local random direction
  float theta = rnd1 * (PI * 2.0f);
  float phi = rnd0 * (PI * 0.5f);
  float sin_phi = sin(phi);
  float3 dir = {sin_phi * cos(theta), cos(phi), sin_phi * sin(theta)};
  // consine weighted
  probability = dir.y;
  // rotate to world space
  float3 tangent = {0, 1, 0};
  if (abs(normal.y) > 0.995f) tangent = float3(normal.z, 0, -normal.x);
  float3 bitangent = cross(tangent, normal);
  tangent = cross(normal, bitangent);
  return dir.x * tangent + dir.y * normal + dir.z * bitangent;
}

  [shader("closesthit")] void closest_hit_shader(inout RayPayload payload, in HitAttributes attr) {
  // hardcoded
  float3 light_dir = {2, 2, 1.5};
  float3 light_color = {0.6, 0.6, 0.6};
  float3 albedo = {0.7, 0.7, 0.7};

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

  // diffusive direct lighting
  float3 l = normalize(light_dir);
  float n_dot_l = max(0.0, dot(n, l));
  float3 direct_light = n_dot_l * light_color;

  // collect bounced irradiance
  float3 bounced = {0, 0, 0};
  if (payload.depth < 2) {
    HaltonState halton;
    uint3 dispatch_index = DispatchRaysIndex();
    halton_init(halton, dispatch_index.xyz);

    float weight_sum = 0;
    const uint c_sample = max(MAX_HALTON_SAMPLE / 2, 4);
    for (int i = 0; i < c_sample; i++) {

      float rnd0 = frac(halton_next(halton));
      float rnd1 = frac(halton_next(halton));
      float weight;
      float3 dir = sample_lambertian_brdf(n, rnd0, rnd1, weight);

      RayDesc ray;
      ray.Origin = world_pos;
      ray.Direction = dir;
      ray.TMin = 0.001;
      ray.TMax = 10000.0;
      RayPayload bounced_payload = {{0, 0, 0, 0}, payload.depth + 1};
      TraceRay(scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 0, 0, ray, bounced_payload);

      bounced += weight * bounced_payload.color.xyz;
      weight_sum += weight;
    }
    bounced /= weight_sum;
  }

  payload.color.xyz = (direct_light + bounced) * albedo;
}

[shader("miss")] void miss_shader(inout RayPayload payload) {
  float3 ayanami_blue = {129.0 / 255, 187.0 / 255, 235.0 / 255};
  float3 asuka_red = {156.0 / 255, 0, 0};
  float3 origin = WorldRayDirection();
  float mixing = clamp(1 - origin.y, 0, 2) / 2;
  mixing = pow(mixing, 3);
  payload.color = float4(lerp(ayanami_blue, asuka_red, mixing), 1.0);
}
