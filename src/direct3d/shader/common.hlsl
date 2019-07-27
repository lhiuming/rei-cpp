#ifndef REI_COMMON_HLSL
#define REI_COMMON_HLSL

#define c_epsilon 1E-38

#define PI 3.141592653589793238462643383279502884197169399375105820974f
#define PI_2 (2 * PI)
#define c_pi PI

float3 gradiant_sky_eva(float3 view_direction) {
  float3 ayanami_blue = {129.0 / 255, 187.0 / 255, 235.0 / 255};
  float3 asuka_red = {156.0 / 255, 0, 0};
  float y = view_direction.y;
  float mixing = clamp(1 - y, 0, 2) / 2;
  mixing = pow(mixing, 3);
  return lerp(ayanami_blue, asuka_red, mixing);
}

#endif