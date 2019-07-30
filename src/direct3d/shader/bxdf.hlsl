#ifndef REI_BXDF_HLSL
#define REI_BXDF_HLSL

/*
 * Utility for BXDF evaluations.
 * 
 * Reference:
 *   [rtr4] <Real-Time Rendering>, 4th Edition, 2018.
 *   [walter2007] <Microfacet Models for Refraction through Rough Surfaces> by Bruce Walter, 2007.
 *     - url: https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
 *   [seb] <Adopting a physically based shading model> by Sébastien Lagarde.
 *     - url: https://seblagarde.wordpress.com/2011/08/17/hello-world/
 */

#include "common.hlsl"

// Functions can just be listed for reference, and may never really be used in shading.

// Schlick's approximated Fresnel reflectance.
// ref: [rtr4] p.321
float F_schlick(float F_0, float dot_n_l) {
  float c = 1 - dot_n_l;
  float c2 = c * c;
  float c5 = c2 * c2 * c;
  return F_0 + (1 - F_0) * c;
}

float3 F_schlick(float3 F_0, float dot_n_l) {
  float c = 1 - dot_n_l;
  float c2 = c * c;
  float c5 = c2 * c2 * c;
  return F_0 + (1 - F_0) * c;
}

// Phong NDF
// ref: [walter2007] 
float D_phong(float dot_r_v, float a_roughness) {
  // NOTE: paramter `a` might be called "glossiness" conventionally
  float a = a_roughness;
  float normalizer = (a + 2) / PI_2;
  return normalizer * pow(dot_r_v, a);
}

// Blinn-Phong NDF modified for enegery conservation
// ref: [seb]
float D_blinn_phong(float dot_m_n, float a_roughness) {
  float a_p = a_roughness;
  float a_b = pow(2, -0.5 * a_p) + a_p;
  float normalizer = (a_p + 2) * (a_p + 4) / (8 * PI * a_b);
  return normalizer * pow(dot_m_n, a_p);
}

// Beckmann NDF
// ref: [rtr4] (and [walter2007])
float D_beckmann(float dot_m_n_clamped, float a_roughness) {
  float dot_m_n = dot_m_n_clamped + c_epsilon;
  float a2 = a_roughness * a_roughness;
  float cos2 = dot_m_n * dot_m_n;
  float cos4 = cos2 * cos2;
  return 1 / (PI * a2 * cos4) * exp((cos2 - 1) / (a2 * cos2));
}

// Smith G1 for Beckmann NDF, with approximation by schlick
// ref: [walter2007]
float G1Smith_beckmann_schlick(float a_roughness) {
  float a = a_roughness;
  if (a >= 1.6) return 1;
  float a2 = a * a;
  return (3.535 * a + 2.181 * a2) / (1 + 2.276 * a + 2.577 * a2);
}

// GGX/Trowbridge-Reitz NDF
// ref: [rtr4]
float D_ggx(float dot_n_l_clamped, float a_roughness) {
  float a = a_roughness;
  float a2 = a * a;
  float dot_n_l = dot_n_l_clamped;
  float cos2 = dot_n_l * dot_n_l;
  // TODO does compiler optimize the pow(x, 2)?
  return a2 / (PI * pow(1 + cos2 * (a2 - 1) + c_epsilon, 2));
}

// Smith G1 for GGX NDF, as approximated by Karis
// ref: [rtr4], p342
float G1Smith_ggx_karis(float dot_n_l_clamped, float a_roughness) {
  float a = a_roughness;
  float dot_n_l = dot_n_l_clamped + c_epsilon;
  return (2 * dot_n_l) / (dot_n_l * (2 - a) + a);
}

// Smith G2 combined with the microfacet denominator 1/(4 * dot_n_l * dot_n_v),
// assumuing height-correlated G2, as approximated by Hammon et al
// ref: [rtr4], p342
float G2Smith_ggx_hammon_devided(float dot_n_l_clamped, float dot_n_v_clamped, float a_roughness) {
  float a = a_roughness;
  float dot_n_l = dot_n_l_clamped + c_epsilon;
  float dot_n_v = dot_n_v_clamped + c_epsilon;
  return 0.5 / lerp(2 * dot_n_l * dot_n_v, dot_n_l + dot_n_v, a);
}

// Lambertian diffusive BRDF
float BRDF_lambertian() {
  return 1 / PI;
}

#endif