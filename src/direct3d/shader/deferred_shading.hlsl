#include "common.hlsl"
#include "deferred_common.hlsl"

struct Light {
  float4 pos_dir;
  float4 color;
};

ConstantBuffer<Light> g_light : register(b0, space0);

ConstantBuffer<ConstBufferPerRender> g_render : register(b0, space1);

// G-buffers
sampler samp : register(s0, space1);
Texture2D<float> depth_buffer : register(t0, space1);
Texture2D<float4> normal_buffer : register(t1, space1);
Texture2D<float4> albedo_smoothness_buffer : register(t2, space1);

float4 VS(uint vid : SV_VertexID) : SV_POSITION {
  /*
   *  2 --------------------- 1
   *  |     .   / |         /
   *  |     . /   |       /
   *  | - - * - - |     /
   *  |   / .     |   /
   *  | /   .     | /
   *  |-----------/
   *  |         /
   *  |       /
   *  |     /
   *  |   /
   *  | /
   *  0 
   */

  if (vid == 0) return float4(-1, -3, 1, 1);
  if (vid == 1) return float4(+3, +1, 1, 1);
  if (vid == 2) return float4(-1, +1, 1, 1);

  return float4(0, 0, 0, 1);
}

struct Surface {
  float3 normal;
  float3 albedo;
  float smoothness;
};

struct PunctualLight {
  float3 color;
  float3 dir;
};

float remap_smoothness(float normalized_value) {
  return 80 * normalized_value;
}

// Blinn-Phong specular and diffuse
float3 blinn_phong(Surface surface, PunctualLight light, float3 ambient, float3 view) {
  float diffuse = saturate(dot(surface.normal, light.dir));

  float3 h = normalize(light.dir + view);
  float smoothness = remap_smoothness(surface.smoothness);
  float specular = pow( saturate( dot(surface.normal, h) ), smoothness);

  float3 lighting = ambient + (diffuse + specular) * light.color;

  return surface.albedo * lighting;
}

float4 PS(float4 pos : SV_POSITION) : SV_TARGET {
  float2 screen_size = g_render.screen.xy;
  float2 uv = pos.xy / screen_size;

  float z = depth_buffer.Sample(samp, uv);
  float is_background = step(z, c_epsilon); 

  Surface surf;
  surf.normal = normal_buffer.Sample(samp, uv).xyz;
  float4 albe_smoo = albedo_smoothness_buffer.Sample(samp, uv);
  surf.albedo = albe_smoo.xyz;
  surf.smoothness = albe_smoo.w;

  // get world pos from depth
  float4 ndc = float4((uv - 0.5) * float2(2.f, -2.f), z, 1);
  float4 w_coord = mul(g_render.camera_view_proj_inv, ndc);
  float3 w_pos = w_coord.xyz / w_coord.w;

  float3 view_dir = normalize(w_pos - g_render.camera_pos.xyz);

  // get background color from sky 
#if BASE_SHADING
  float3 bg_color = gradiant_sky_eva(view_dir);
  float3 ambient = gradiant_sky_eva(surf.normal) * 0.3f;
#else
  float3 bg_color = float3(0, 0, 0);
  float3 ambient = float3(0, 0, 0);
#endif

  PunctualLight light;
  if (g_light.pos_dir.w > 0.5) { // position
    light.dir = normalize(w_pos - g_light.pos_dir.xyz);
  } else { // direction
    light.dir = g_light.pos_dir.xyz;
  }
  light.color = g_light.color.xyz;

  // Bliin-Phong reflection with Lambertian Diffuse
  float3 radiance = blinn_phong(surf, light, ambient, -view_dir);

  float3 final_color = lerp(radiance, bg_color, is_background);
  return float4(final_color, 1.0f);
}
