#ifndef REI_COMMON_HLSL
#define REI_COMMON_HLSL

#define EPS 1E-23
#define c_epsilon EPS

#define PI 3.141592653589793238462643383279502884197169399375105820974f
#define PI_INV (1/3.141592653589793238462643383279502884197169399375105820974f)
#define PI_2 (2 * PI)
#define c_pi PI
#define c_pi_2 PI_2
#define c_pi_inv PI_INV

// Characteristic function
float chara(float a) {
  return a > 0 ? 1 : 0;
}

float max3(float3 v) {
  return max(v.x, max(v.y, v.z));
}

bool isnan3(float3 v) {
  return isnan(v.x) || isnan(v.y) || isnan(v.z);
}

#endif