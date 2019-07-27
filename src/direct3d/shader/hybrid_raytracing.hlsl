#include "common.hlsl"
#include "halton.hlsl"
#include "hybrid_common.hlsl"

struct Vertex {
  // NOTE: see VertexElement in cpp code
  float4 pos;
  float4 color;
  float3 normal;
};

// Global //

// Output buffer
RWTexture2D<float4> g_render_target : register(u0, space0);

// Scene TLAS
RaytracingAccelerationStructure g_scene : register(t0, space0);
// G-Buffer
Texture2D<float> g_depth : register(t1, space0);
Texture2D<float4> g_normal : register(t2, space0);
Texture2D<float4> g_albedo : register(t3, space0);
Texture2D<float4> g_emissive: register(t4, space0);

// Per-render CB
ConstantBuffer<PerRenderConstBuffer> g_per_render : register(b0, space0);

// Local: Hit Group //

//  mesh data
ByteAddressBuffer g_indicies : register(t0, space1);
StructuredBuffer<Vertex> g_vertices : register(t1, space1);

// inplace material data
ConstantBuffer<PerMaterialConstBuffer> g_material : register(b0, space1);

// Raytracing Structs //

typedef BuiltInTriangleIntersectionAttributes HitAttr;

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

float4 sample_sky(float3 dir) {
  return float4(gradiant_sky_eva(dir), 1.0f);
}

void output(float4 color) {
  g_render_target[DispatchRaysIndex().xy] = color;
}

void output(float3 color) {
  output(float4(color, 1.0));
}

struct Surface {
  float3 normal;
  float smoothness;
  float3 albedo;
};

struct BRDFSample {
  float3 wi;
  float weight;
};

float remap_smoothness(float normalized_value) {
  return 80 * normalized_value;
}

void sample_blinn_phong_brdf(Surface surf, float3 wo, float rnd0, float rnd1, out BRDFSample ret) {
  // compute local random direction
  // NOTE: for a uniform distribution on hemisphere, phi = acos(rnd1) 
  float theta = rnd0 * PI_2;
  float cos_phi = rnd1;
  float sin_phi = sqrt(1 - cos_phi * cos_phi);
  float3 dir = {sin_phi * cos(theta), cos_phi, sin_phi * sin(theta)};
  float importance = 1.f;

  // rotate to world space
  float3 normal = surf.normal;
  float3 tangent = wo;
  if (dot(normal, tangent) > (1 - c_epsilon)) tangent = normalize(float3(normal.z, 0, -normal.x));
  float3 bitangent = cross(tangent, normal);
  tangent = cross(normal, bitangent);

  // TODO nomrmlaized might be redudant
  ret.wi = normalize(dir.x * tangent + dir.y * normal + dir.z * bitangent);

  // evaluate brdf weight: lambertian constant lobe + blinn-phong tilted lobe
  float3 h = normalize(ret.wi + wo);
  float smoothness = remap_smoothness(surf.smoothness);
  // NOTE: 1-surf.smoothness to fake energy conservation
  // NOTE: normalization factor is ignored
  float brdf = (1-surf.smoothness) + PI * pow(dot(h, normal), smoothness) / cos_phi; 

// irradiance: consine projection
  float wi_proj = cos_phi;

  ret.weight = (brdf * wi_proj) * importance;
}

float3 integrate_blinn_phong(in float3 pos, in float3 wo, in Surface surf, float recur_depth) {
  HaltonState halton;
  uint3 dispatch_index = DispatchRaysIndex();
  halton_init(halton, dispatch_index.xyz);

  RayDesc ray;
  ray.Origin = pos;
  ray.TMin = 0.001;
  ray.TMax = 10000.0;

  float3 accumulated = float3(0, 0, 0);
  float weight_sum = 0;

  const uint c_sample = max(8, MAX_HALTON_SAMPLE / 2);
  BRDFSample samp;
  RayPayload payload = {{0, 0, 0, 0}, recur_depth + 1};
  for (int i = 0; i < c_sample; i++) {
    float rnd0 = frac(halton_next(halton));
    float rnd1 = frac(halton_next(halton));
    sample_blinn_phong_brdf(surf, wo, rnd0, rnd1, samp);

    ray.Direction = samp.wi;
    TraceRay(g_scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 0, 0, ray, payload);

    accumulated += samp.weight * payload.color.xyz;
    weight_sum += samp.weight;
  }

  return (accumulated / weight_sum) * surf.albedo;
}

[shader("raygeneration")] void raygen_shader() {
  uint2 id = DispatchRaysIndex().xy;
  float depth = g_depth[id];

  float2 uv = float2(id) / float2(DispatchRaysDimensions().xy);
  float4 ndc = float4(uv * 2.f - 1.f, depth, 1.0f);
  ndc.y = -ndc.y;
  float4 world_pos_h = mul(g_per_render.proj_to_world, ndc);
  float3 world_pos = world_pos_h.xyz / world_pos_h.w;

  float3 wo = normalize(g_per_render.camera_pos.xyz - world_pos);

  // reject on background
  if (depth <= c_epsilon) {
    float3 view_dir = -wo;
    output(sample_sky(view_dir));
    return;
  }

  float4 normal = g_normal[id];
  float4 albedo_smoothness = g_albedo[id];
  float4 emissive = g_emissive[id];

  Surface s;
  s.normal = g_normal[id].xyz;
  s.smoothness = albedo_smoothness.w;
  s.albedo = albedo_smoothness.xyz;
  float3 emittance = s.albedo * emissive.x;
  float3 radiance = emittance + integrate_blinn_phong(world_pos, wo, s, 0);

  output(radiance);
}

[shader("closesthit")] void closest_hit_shader(inout RayPayload payload, in HitAttr attr) {
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

  // collect bounced irradiance
  float3 radiance;
  if (payload.depth < 2) {
    // Bounc around the scene !
    float3 world_pos = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    float3 wo = -WorldRayDirection();

    // TODO read material from shader table
    Surface surf;
    surf.normal = n;
    surf.smoothness = get_smoothness(g_material);
    surf.albedo = get_albedo(g_material);
    float emissive = get_emissive(g_material);
    float3 emittance = emissive * surf.albedo; // fake
    radiance = emittance + integrate_blinn_phong(world_pos, wo, surf, payload.depth);
  } else {
    // kill the path
    float ambient = 0.001;
    radiance = float3(1, 1, 1) * ambient;
  }

  payload.color.xyz = radiance;
}

  [shader("miss")] void miss_shader(inout RayPayload payload) {
  float3 dir = WorldRayDirection();
  payload.color = float4(gradiant_sky_eva(dir), 1.0f);
}
