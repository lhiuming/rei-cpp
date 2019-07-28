#ifndef REI_HALTON_HLSL
#define REI_HALTON_HLSL

/*
 * Halton sequence generator.
 * Implemented as a global sampler on a [256x256]-pixel tile.
 *
 * Modified from "Hybrid Rendering for Real-Time Ray Tracing" by SEED/EA, from <Ray Tracing Gems>.
 * Also see [pbrt]<Physically Based Rendering: From Theory to Implementation> for reference on some re-wording.
 */

// Max 2d sample number is limited by the prime number sequence length, see halton_sample(...)
#define c_halton_max_sample_2d uint((46 - 2) / 2)
#define HALTON_MAX_SAMPLE_2D c_halton_max_sample_2d

struct HaltonState {
  uint dimension;
  uint sequence_index;
};

uint halton_2inverse(uint index, uint digits) {
  index = (index << 16) | (index >> 16);
  index = ((index & 0x00ff00ff) << 8) | ((index & 0xff00ff00) >> 8);
  index = ((index & 0x0f0f0f0f) << 4) | ((index & 0xf0f0f0f0) >> 4);
  index = ((index & 0x33333333) << 2) | ((index & 0xcccccccc) >> 2);
  index = ((index & 0x55555555) << 1) | ((index & 0xaaaaaaaa) >> 1);
  return index >> (32 - digits);
}

uint halton_3inverse(uint index, uint digits) {
  uint result = 0;
  for (uint d = 0; d < digits; ++d) {
    result = result * 3 + index % 3;
    index /= 3;
  }
  return result;
}

uint halton_index(uint x, uint y, uint i) {
  const uint2 tile_size = {256, 256};
  const uint2 base_scale = {256, 729}; // determined from tile_size, on base-2 and base-3
  const uint2 base_exp = {8, 6};       // determined from tile_size, on base-2 and base-3
  // mult_inv: determined from from base_scale using extended GCD; see [pbrt] for reference
  const uint2 mult_inv = {105, 430};
  const uint sample_stride = base_scale.x * base_scale.y;
  const uint2 offset_mul = sample_stride / base_scale * mult_inv;
  uint big_prime = 186624;

  uint offset_x = halton_2inverse(x % tile_size.x, base_exp.x) * offset_mul.x;
  uint offset_y = halton_3inverse(y % tile_size.y, base_exp.y) * offset_mul.y;
  return (offset_x + offset_y) % sample_stride + i * big_prime;
}

void halton_init(
  inout HaltonState hState, int x, int y, int path, int num_paths, int frame_id, int loop) {
  // start from dim 2; the first 2 dimension is encode in index; see halton_index(...)
  hState.dimension = 2;
  hState.sequence_index = halton_index(x, y, (frame_id * num_paths + path) % (loop * num_paths));
}

HaltonState halton_init(int x, int y, int path, int num_paths, int frame_id, int loop) {
  HaltonState state;
  halton_init(state, x, y, path, num_paths, frame_id, loop);
  return state;
}

HaltonState halton_init(uint2 p, uint frame_id, uint frame_loop) {
  return halton_init(p.x, p.y, 0, 1, frame_id, frame_loop);
}

float halton_sample(uint dimension, uint index) {
  // Choose a prime radix-base
  int base = 0;
  switch (dimension) {
    // clang-format off
    case 0: base = 2; break;
    case 1: base = 3; break;
    case 2: base = 5; break;
    case 3: base = 7; break;
    case 4: base = 11; break;
    case 5: base = 13; break;
    case 6: base = 17; break;
    case 7: base = 19; break;
    case 8: base = 23; break;
    case 9: base = 29; break;
    case 10: base = 31; break;
    case 11: base = 37; break;
    case 12: base = 41; break;
    case 13: base = 43; break;
    case 14: base = 47; break;
    case 15: base = 53; break;
    case 16: base = 59; break;
    case 17: base = 61; break;
    case 18: base = 67; break;
    case 19: base = 71; break;
    case 20: base = 73; break;
    case 21: base = 79; break;
    case 22: base = 83; break;
    case 23: base = 89; break;
    case 24: base = 97; break;
    case 25: base = 101; break;
    case 26: base = 103; break;
    case 27: base = 107; break;
    case 28: base = 109; break;
    case 29: base = 113; break;
    case 30: base = 127; break;
    case 31: base = 131; break;
    case 32: base = 137; break;
    case 33: base = 139; break;
    case 34: base = 149; break;
    case 35: base = 151; break;
    case 36: base = 157; break;
    case 37: base = 163; break;
    case 38: base = 167; break;
    case 39: base = 173; break;
    case 40: base = 179; break;
    case 41: base = 181; break;
    case 42: base = 191; break;
    case 43: base = 193; break;
    case 44: base = 197; break;
    case 45: base = 199; break;
    default: base = 2; break;
      // clang-format on
  }

  // Calculate radical inverse
  float a = 0;
  float inv_base = 1.0f / float(base);
  for (float mult = inv_base; index != 0; index /= base, mult *= inv_base) {
    a += float(index % base) * mult;
  }

  return a;
}

float halton_next(inout HaltonState state) {
  return halton_sample(state.dimension++, state.sequence_index);
}

#endif
