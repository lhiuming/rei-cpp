#ifndef REI_BLUE_NOISE_HLSL
#define REI_BLUE_NOISE_HLSL

/*
 * Procedural blue noise, modifed from https://www.shadertoy.com/view/4dS3Wd
 */

#define NUM_NOISE_OCTAVES 5

float hash(float n) { return frac(sin(n) * 1e4); }
float hash(float2 p) { return frac(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x)))); }

float blue_noise(float x) {
    float i = floor(x);
    float f = frac(x);
    float u = f * f * (3.0 - 2.0 * f);
    return lerp(hash(i), hash(i + 1.0), u);
}

float blue_noise(float2 x) {
  float2 i = floor(x);
  float2 f = frac(x);

  // Four corners in 2D of a tile
  float a = hash(i);
  float b = hash(i + float2(1.0, 0.0));
  float c = hash(i + float2(0.0, 1.0));
  float d = hash(i + float2(1.0, 1.0));

  // Simple 2D lerp using smoothstep envelope between the values.
  // return vec3(mix(mix(a, b, smoothstep(0.0, 1.0, f.x)),
  //			mix(c, d, smoothstep(0.0, 1.0, f.x)),
  //			smoothstep(0.0, 1.0, f.y)));

  // Same code, with the clamps in smoothstep and common subexpressions
  // optimized away.
  float2 u = f * f * (3.0 - 2.0 * f);
  return lerp(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float fbm(float x) {
  float v = 0.0;
  float a = 0.5;
  float shift = float(100);
  for (int i = 0; i < NUM_NOISE_OCTAVES; ++i) {
    v += a * blue_noise(x);
    x = x * 2.0 + shift;
    a *= 0.5;
  }
  return v;
}

float fbm(float2 x) {
  float v = 0.0;
  float a = 0.5;
  float2 shift = 100;
  // Rotate to reduce axial bias
  float2x2 rot = float2x2(cos(0.5), sin(0.5), -sin(0.5), cos(0.50));
  for (int i = 0; i < NUM_NOISE_OCTAVES; ++i) {
    v += a * blue_noise(x);
    x = mul(rot, x) * 2.0 + shift;
    a *= 0.5;
  }
  return v;
}

//
// Some extentions
//

// screen pos input
float blue_noise(uint2 xy) {
  return fbm(float2(xy) / 128);
}

// screen pos input, generating 2 sample
float2 blue_noise2(uint2 xy) {
  float2 s = float2(xy) / 31;
  float2 r = float2(s.y, -s.x) + float2(31.9, 23);
  float n0 = fbm(s);
  float n1 = fbm(r);
  return float2(n0, n1);
}

#endif
