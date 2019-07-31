#ifndef REI_RASTER_COMMON_HLSL
#define REI_RASTER_COMMON_HLSL

//
// Screen pass helper

/*
 * For screen-corvering triangle, mapping of index to position is like below:
 *
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
 *
 */

float4 make_screen_triangle_ndc(uint index) {
  if (index == 0) return float4(-1, -3, 1, 1);
  if (index == 1) return float4(+3, +1, 1, 1);
  if (index == 2) return float4(-1, +1, 1, 1);
  return 0;
}

float2 make_screen_triangle_uv(uint index) {
  // ASSUME texcoord start from upper left
  if (index == 0) return float2(0, 2);
  if (index == 1) return float2(2, 0);
  if (index == 2) return float2(0, 0);
  return 0;
}

#endif