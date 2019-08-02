#include "common.hlsl"
#include "hybrid_common.hlsl"
#include "raster_common.hlsl"
#include "blue_noise.hlsl"

struct SphericalLight {
  float4 pos_radius; // for dir, w is zero; for pos, w is 1.
  float4 color;   // hdr color, defined as irradiance * PI, aka lambertian response
// addional sampling data
  float4 noise_offset;
};

// Per light data
ConstantBuffer<SphericalLight> g_light : register(b0, space0);

// G-buffers
Texture2D<float> g_depth : register(t0, space1);
Texture2D<float4> g_rt0 : register(t1, space1);
Texture2D<float4> g_rt1 : register(t2, space1);
Texture2D<float4> g_rt2 : register(t3, space1);

// Output
RWTexture2D<float4> g_stochastic_radiance: register(u0, space1);
RWTexture2D<float4> g_stochastic_sample_ray: register(u1, space1);
RWTexture2D<float4> g_stochastic_sample_radiance: register(u2, space1);

// Per render info
ConstantBuffer<PerRenderConstBuffer> g_render : register(b0, space1);

// uniform sample on the projected disk
float3 shpere_sample(float3 center, float radius, float3 viewer, float rnd0, float rnd1) {
  float3 axis_y = normalize(viewer - center);
  float3 pre_z = float3(0, 1, 0);
  if (abs(axis_y.y) >= (1 - EPS)) {
    pre_z = float3(1, 0, 0);
  }
  float3 axis_x = normalize(cross(axis_y, pre_z));
  float3 axis_z = cross(axis_x, axis_y); 
  float a = sqrt(clamp(rnd0, EPS, 1));
  float theta = rnd1 * PI_2;
  float r = radius * a;
  float x = r * cos(theta);
  float y = r * sin(theta);
  float z = sqrt(radius * radius - x * x - y * y);
  float3 p = center + axis_x * x + axis_y * y + axis_z * z;
  return p;
}

[numthreads(8, 8, 1)] void CS(uint3 tid
                              : SV_DispatchThreadId) {
  float z = g_depth[tid.xy];

  // skip backgroug pixel
  if (z <= EPS) { return; }

  // get world pos from depth
  float2 uv = tid.xy / get_screen_size(g_render);
  float4 ndc = float4((uv - 0.5) * float2(2.f, -2.f), z, 1);
  float4 w_coord = mul(get_view_proj_inv(g_render), ndc);
  float3 w_pos = w_coord.xyz / w_coord.w;

  // Take a random light position
  float2 blue_rnd = blue_noise2(tid.xy);
  float2 rnd = frac(g_light.noise_offset.zw + blue_rnd);
  float3 p = shpere_sample(g_light.pos_radius.xyz, g_light.pos_radius.w, w_pos, rnd.x, rnd.y);
  float3 delta = p - w_pos;
  float dist2 = dot(delta, delta);
  float dist = sqrt(dist2);
  float3 light_dir = delta / dist;
  float3 light_color = g_light.color.xyz / dist2;

  // Evalute BRDF for surface
  float4 rt0 = g_rt0[tid.xy];
  float4 rt1 = g_rt1[tid.xy];
  float4 rt2 = g_rt2[tid.xy];
  Surface surf = decode_gbuffer(rt0, rt1, rt2);
  float non_grazing = dot(light_dir, surf.normal);
  if (non_grazing < EPS) { 
    // no need to sample
    return;
  }

  BXDFSurface bxdf = to_bxdf(surf);
  float3 viewer_dir = normalize(g_render.camera_pos.xyz - w_pos);
  BrdfCosine brdf_cos = BRDF_GGX_Lambertian(bxdf, viewer_dir, light_dir);
  float3 reflectance = (brdf_cos.specular + brdf_cos.diffuse) * light_color;

  // output
  float3 ray_dir = light_dir;
  float ray_start = 0.02 / non_grazing; // to avoid self intrusion
  float ray_end = dist;
  g_stochastic_radiance[tid.xy] += float4(reflectance, 0.0);
  g_stochastic_sample_ray[tid.xy] = float4(ray_dir, ray_start);
  g_stochastic_sample_radiance[tid.xy] = float4(reflectance, ray_end);
}
