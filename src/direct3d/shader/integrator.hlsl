#ifndef REI_INTEGRATOR_HLSL
#define REI_INTEGRATOR_HLSL

#include "common.hlsl"
#include "halton.hlsl"

/*
 * Integrating/Sampling utililties for ray-tracing.
 */

struct SphereSample{
  float3 wi;
  float importance;
};

struct UniformHSampler {
  HaltonState halton;
  float3 normal;
  float3 tangent;
  float3 bitangent;
};

UniformHSampler uniform_h_sampler_init(in HaltonState halton, float3 normal, float3 pre_tangent) {
  UniformHSampler ret;
  ret.halton = halton;
  // Create a local frame
  ret.normal = normal;
  if (abs(dot(normal, pre_tangent)) >= (1 - EPS)) {
    pre_tangent = float3(pre_tangent.z, 0, -pre_tangent.x);
  }
  ret.bitangent = normalize(cross(pre_tangent, normal));
  ret.tangent = cross(normal, ret.bitangent);
  return ret;
}

SphereSample sampler_next(UniformHSampler s) {
  float rnd0 = frac(halton_next(s.halton));
  float rnd1 = frac(halton_next(s.halton));

  SphereSample ret;

  // compute local random direction
  // NOTE: for a uniform distribution on hemisphere, phi = acos(rnd1)
  // ref: http://mathworld.wolfram.com/SpherePointPicking.html
  float theta = rnd0 * PI_2;
  float cos_phi = rnd1;
  float sin_phi = sqrt(1 - cos_phi * cos_phi);
  float3 dir = {sin_phi * cos(theta), cos_phi, sin_phi * sin(theta)};
  // we are sampling uniformly on hemisphere using sphere coordinate
  ret.importance = PI_2;

  // rotate to world space
  ret.wi = dir.x * s.tangent + dir.y * s.normal + dir.z * s.bitangent;
  return ret;
}

#endif