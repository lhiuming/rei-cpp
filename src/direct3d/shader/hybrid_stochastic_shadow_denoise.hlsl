#include "common.hlsl"
#include "hybrid_common.hlsl"

/*
 * Modefied from <Combining Analytic Direct Illumination and Stochastic Shadows> by Heitz, Hill, and McGuire.
 *   - url: https://eheitzresearch.wordpress.com/705-2/
 */


// For debug drawing
#define FINAL_PASS_ADDICTIVE 1

// Bilateral filter parameters
#define DEPTH_WEIGHT 1.0
#define NORMAL_WEIGHT 1.5
#define PLANE_WEIGHT 1.5
#define ANALYTIC_WEIGHT 0.09
#ifndef R
#define R 5
#endif

#if FINAL_PASS
#define axis int2(0, 1)
#else
#define axis int2(1, 0)
#endif

Texture2D<float4> g_source0 : register(t0, space0);
Texture2D<float4> g_source1 : register(t1, space0);
#if FINAL_PASS
RWTexture2D<float4> g_result: register(u0, space0);
#if FINAL_PASS_DEBUG
RWTexture2D<float4> g_result_ratio: register(u1, space0);
#endif
#else
RWTexture2D<float4> g_result0 : register(u0, space0);
RWTexture2D<float4> g_result1 : register(u1, space0);
#endif

Texture2D<float> g_depth : register(t0, space1);
Texture2D<float4> g_normal: register(t1, space1);
Texture2D<float4> g_analytic : register(t2, space1);
//Texture2D<float4> g_variance : register(t2, space1);

ConstantBuffer<PerRenderConstBuffer> g_render : register(b0, space1);

struct TapKey {
  // camera space depth (linear) 
  float csZ;
  // position and normal in either world space or camera space
  float3 position;
  float3 normal;
  float analytic;
};

float mean(float3 v) {
  return (v.x + v.y + v.z) / 3;
}

TapKey getTapKey(uint2 ss_coord) {
  float depth = g_depth[ss_coord];

  float2 uv = float2(ss_coord) / float2(get_screen_size(g_render));
  float4 ndc = float4(uv * 2.f - 1.f, depth, 1.0f);
  ndc.y = -ndc.y;
  float4 cs_pos_h = mul(get_proj_inv(g_render), ndc);
  float3 cs_pos = cs_pos_h.xyz / cs_pos_h.w;
  float4 world_pos_h = mul(g_render.proj_to_world, ndc);
  float3 world_pos = world_pos_h.xyz / world_pos_h.w;

  TapKey key;
  if ((DEPTH_WEIGHT != 0.0) || (PLANE_WEIGHT != 0.0)) { key.csZ = -cs_pos.z; }

  if (PLANE_WEIGHT != 0.0) {
    key.position = world_pos;
  }

  if ((NORMAL_WEIGHT != 0.0) || (PLANE_WEIGHT != 0.0)) {
    key.normal = get_normal(g_normal[ss_coord]);
  }

  if (ANALYTIC_WEIGHT != 0.0) { key.analytic = mean(g_analytic[ss_coord].rgb); }

  return key;
}

float bilateral_weight(TapKey center, TapKey tap) {
    float depthWeight   = 1.0;
    float normalWeight  = 1.0;
    float planeWeight   = 1.0;
    float analyticWeight  = 1.0;

    if (DEPTH_WEIGHT != 0.0) {
        depthWeight = max(0.0, 1.0 - abs(tap.csZ - center.csZ) * DEPTH_WEIGHT);
    }

    if (NORMAL_WEIGHT != 0.0) {
        float normalCloseness = dot(tap.normal, center.normal);
        normalCloseness = normalCloseness*normalCloseness;
        normalCloseness = normalCloseness*normalCloseness;

        float normalError = (1.0 - normalCloseness);
        normalWeight = max((1.0 - normalError * NORMAL_WEIGHT), 0.00);
    }

    if (PLANE_WEIGHT != 0.0) {
        float lowDistanceThreshold2 = 0.001;

        // Change in position in camera space
        float3 dq = center.position - tap.position;

        // How far away is this point from the original sample
        // in camera space? (Max value is unbounded)
        float distance2 = dot(dq, dq);

        // How far off the expected plane (on the perpendicular) is this point?  Max value is unbounded.
        float planeError = max(abs(dot(dq, tap.normal)), abs(dot(dq, center.normal)));

        planeWeight = (distance2 < lowDistanceThreshold2) ? 1.0 :
            pow(max(0.0, 1.0 - 2.0 * PLANE_WEIGHT * planeError / sqrt(distance2)), 2.0);
    }

    if (ANALYTIC_WEIGHT != 0.0) {
        float aDiff = abs(tap.analytic - center.analytic) * 10.0;
        analyticWeight = max(0.0, 1.0 - (aDiff * ANALYTIC_WEIGHT));
    }

    return depthWeight * normalWeight * planeWeight * analyticWeight;
}

// TODO optimize this using group shared memory
[numthreads(8, 8, 1)] 
void CS(uint3 tid : SV_DispatchThreadId) {
  uint2 ss_coord = tid.xy;

  // TODO add estimate
  // 3* is because the estimator produces larger values than we want to use, for visualization
  // purposes
  // float gaussian_radius = saturate(texelFetch(noiseEstimate, ssC, 0).r * 1.5) * R;
  float gaussian_radius = 1.5  * R;

  float depth = g_depth[ss_coord];

  float3 result0, result1;
  // Detect sky and noiseless pixels and reject them
  bool skip_denoise = depth <= EPS || gaussian_radius <= 0.5;
  if (skip_denoise) {
    result0 = g_source0[ss_coord].rgb;
    result1 = g_source1[ss_coord].rgb;
  } else {
    float3 sum0 = 0;
    float3 sum1 = 0;
    float total_weight = 0.0;
    TapKey key = getTapKey(ss_coord);

    for (int r = -R; r <= R; ++r) {
      uint2 tapOffset = int2(axis * r);
      uint2 tapLoc = ss_coord + tapOffset;

      float _a = float(r) / gaussian_radius;
      float gaussian = exp(-(_a * _a));
      float weight = gaussian * ((r == 0) ? 1.0 : bilateral_weight(key, getTapKey(tapLoc)));
      sum0 += g_source0[tapLoc].rgb * weight;
      sum1 += g_source1[tapLoc].rgb * weight;
      total_weight += weight;
    }

    // totalWeight >= gaussian[0], so no division by zero here
    result0 = sum0 / total_weight;
    result1 = sum1 / total_weight;
  }

#if FINAL_PASS
  float3 result_ratio;
  for (int i = 0; i < 3; ++i) {
    // Threshold degenerate values
    result_ratio[i] = (result1[i] < 0.0001) ? 1.0 : (result0[i] / result1[i]);
  }
  float3 result = result_ratio * g_analytic[ss_coord];
#if FINAL_PASS_ADDICTIVE
  g_result[tid.xy] += float4(result, 0);
#else
  g_result[tid.xy] = float4(result, 0);
#endif
#if FINAL_PASS_DEBUG
  g_result_ratio[tid.xy] = float4(result_ratio, 0);
#endif
#else
  g_result0[ss_coord] = float4(result0, 0);
  g_result1[ss_coord] = float4(result1, 0);
#endif
}
