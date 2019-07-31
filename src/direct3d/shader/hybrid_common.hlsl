#ifndef REI_HYBRID_COMMON_HLSL
#define REI_HYBRID_COMMON_HLSL

#include "bxdf.hlsl"
#include "common.hlsl"

//
// Encoding&Decoding
//

#define FRAME_LOOP uint(256)
#define c_frame_loop FRAME_LOOP

struct PerRenderConstBuffer {
  // NOTE: see PerFrameConstantBuffer in cpp code
  float4 screen;
  float4x4 view_proj;
  float4x4 proj_to_world;
  float4 camera_pos;
  float4 render_info;
};

float2 get_screen_size(PerRenderConstBuffer cb) {
  return cb.screen.xy;
}

float4x4 get_view_proj(PerRenderConstBuffer cb) {
  return cb.view_proj;
}

float4x4 get_view_proj_inv(PerRenderConstBuffer cb) {
  return cb.proj_to_world;
}

uint get_frame_id(PerRenderConstBuffer cb) {
  return uint(cb.render_info.x);
}

struct PerMaterialConstBuffer {
  float4 albedo;
  float4 sme_unused;
};

float3 get_color(in PerMaterialConstBuffer mat) {
  return mat.albedo.xyz;
}
float get_smoothness(in PerMaterialConstBuffer mat) {
  return mat.sme_unused.x;
}
float get_metalness(in PerMaterialConstBuffer mat) {
  return mat.sme_unused.y;
}
float3 get_emissive(in PerMaterialConstBuffer mat) {
  return mat.sme_unused.z * get_color(mat);
}

struct Surface {
  float3 normal;
  float smoothness;
  float3 color;
  float metalness;
  float3 emissive;
};

// Fill surface properties from Mateiral constance buffer
void fill_surface(PerMaterialConstBuffer cb, float3 normal, inout Surface surf) {
  surf.normal = normal;
  surf.smoothness = get_smoothness(cb);
  surf.color = get_color(cb);
  surf.metalness = get_metalness(cb);
  surf.emissive = get_emissive(cb);
}

Surface decode_material(PerMaterialConstBuffer cb, float3 normal) {
  Surface surf;
  fill_surface(cb, normal, surf);
  return surf;
}

struct GBufferPixel {
  float4 normal_smoothness : SV_TARGET0;
  float4 color_metalness : SV_TARGET1;
  float4 emissive : SV_TARGET2;
};

GBufferPixel encode_gbuffer(PerMaterialConstBuffer mat, float3 normal) {
  GBufferPixel rt;
  rt.normal_smoothness.xyz = normalize(normal);
  rt.normal_smoothness.w = get_smoothness(mat);
  rt.color_metalness.xyz = get_color(mat);
  rt.color_metalness.w = get_metalness(mat);
  rt.emissive.xyz = get_emissive(mat);
  return rt;
}

// Fill surface properties from GBuffer
void fill_surface(float4 rt0, float4 rt1, float4 rt2, inout Surface s) {
  s.normal = rt0.xyz;
  s.smoothness = rt0.w;
  s.color = rt1.xyz;
  s.metalness = rt1.w;
  s.emissive = rt2.xyz;
}

// Read surface properties from GBuffer
Surface decode_gbuffer(float4 rt0, float4 rt1, float4 rt2) {
  Surface surf;
  fill_surface(rt0, rt1, rt2, surf);
  return surf;
}

//
// Shading
//

struct BXDFSurface {
  float3 normal;
  float roughness_percepted;
  float3 fresnel_0;
  float3 albedo;
};

BXDFSurface to_bxdf(Surface surf) {
  BXDFSurface bxdf;
  bxdf.normal = surf.normal;
  bxdf.roughness_percepted = saturate(1 - surf.smoothness);
  bxdf.fresnel_0 = lerp(0.03, surf.color, surf.metalness);
  bxdf.albedo = lerp(surf.color, 0, surf.metalness);
  return bxdf;
}

struct BrdfCosine {
  float3 specular;
  float3 diffuse;
};

// Classic Blinn-Phong specular brdf
// NOTE: some normalization is applied here, ref:
// https://seblagarde.wordpress.com/2011/08/17/hello-world/
BrdfCosine blinn_phong_classic(BXDFSurface surf, float3 wo /*view vector*/, float3 wi /*light vector*/) {
  BrdfCosine ret;
  // <Call of Duty: BlackOps> mapping
  float smoothness = 1 - surf.roughness_percepted;
  float a = pow(8192, smoothness);
  float3 h = normalize(wo + wi); // fixme redudant normalization
  float dot_n_h = clamp(dot(surf.normal, h), c_epsilon, 1);
  float dot_n_l = clamp(dot(surf.normal, wi), c_epsilon, 1);
  float3 color = surf.albedo + surf.fresnel_0 - 0.03;
  float normalizer = (a + 2) / (4 * PI * (2 - pow(2, -0.5 * a)));
  float ks = smoothness; // fake enery conservation
  float kd = 1 - smoothness;
  ret.specular = ks * color * pow(dot_n_h, a) * normalizer;
  ret.diffuse = kd * color * dot_n_l * (1 - smoothness);
  return ret;
}

// Energy Consering Blinn-Phong BRDF simulated with Beckmann Lambda function, with roughness
// remapped accoordingly. ref: [rtr4], p340
BrdfCosine BRDF_blinn_phong_modified(BXDFSurface surf, float3 wo /*view vector*/, float3 wi /*light vector*/) {
  BrdfCosine ret;
  // <Call of Duty: BlackOps> mapping
  float a = pow(8192, 1 - surf.roughness_percepted);
  // Remap to beckmann roughness, and evalue G2
  float3 h = normalize(wo + wi);
  float dot_v_n = dot(wo, surf.normal);
  float dot_v_n_2 = dot_v_n * dot_v_n;
  float a_b_for_G = sqrt((0.5 * a + 1) / (1 - dot_v_n_2)) * dot_v_n;
  // Evalute BRDF
  float dot_l_h = dot(wi, h);
  float dot_l_n = dot(wi, surf.normal);
  float dot_h_n = max(dot(h, surf.normal), EPS); // clamp to avoid nan in self-intrusion
  float3 f = F_schlick(surf.fresnel_0, dot_l_h);
  float g1 = G1Smith_beckmann_schlick(a_b_for_G);
  float g2 = g1 * g1;
  float d = D_blinn_phong(dot_h_n, a);
  // NOTE: 1/dot_l_n is canceld with irradiance projection
  ret.specular = f * g2 * d / (4 * dot_v_n);
  // perfect scattering by 1 - f
  ret.diffuse = (1 - f) * surf.albedo * BRDF_lambertian() * dot_l_n;
  return ret;
}

BrdfCosine BRDF_GGX_Lambertian(BXDFSurface surf, float3 wo /*view vector*/, float3 wi /*light vector*/) {
  BrdfCosine ret;
  // Disney mapping, ref: [rtr4]
  float _a = surf.roughness_percepted;
  float a = _a * _a;
  // Evalute BRDF
  float3 h = normalize(wo + wi);
  float dot_l_h = dot(wi, h);
  float3 f = F_schlick(surf.fresnel_0, dot_l_h);
  float dot_l_n = max(dot(wi, surf.normal), EPS); // avoid nan in G2
  float dot_v_n = max(dot(wo, surf.normal), EPS); // avoid nan in G2
  float g2_denom = G2Smith_ggx_hammon_devided(dot_l_n, dot_v_n, a);
  float dot_h_n = max(dot(h, surf.normal), EPS); // avoid nan in d, due to self-intrusion
  float d = D_ggx(dot_h_n, a);
  // NOTE: denominator part 1 / (4 * dot_l_n * dot_v_n) is pre-multiplied in g2_denom
  ret.specular = f * d * g2_denom * dot_l_n;
  // perfect scattering by 1 - f
  ret.diffuse = (1 - f) * surf.albedo * BRDF_lambertian() * dot_l_n;
  return ret;
}

#endif