#ifndef REI_HALTON_HLSL
#define REI_HALTON_HLSL

#define MAX_HALTON_SAMPLE 8

/*
 * Halton sequence generator.
 *
 * Modified from "Hybrid Rendering for Real-Time Ray Tracing" by SEED/EA, from <Ray Tracing Gems>.
 */

struct HaltonState {
  uint dimension;
  uint sequence_index;
};

// TODO improve this seeding
uint haltonIndex(uint x, uint y) {
  return ~(x % 256 << 8) * 765 + ~((y % 256)) * 110080 + (x * y - 223);
}

void halton_init(inout HaltonState state, uint x, uint y) {
  state.dimension = 2;
  // TODO improce seeding
  state.sequence_index = haltonIndex(x, y) % 912; 
}

void halton_init(inout HaltonState state, uint3 v) {
  halton_init(state, v.x, v.y);
}

float halton_sample(uint dimension, uint index) {
  int base = 0;

  // Choose a prime
  // clang-format off
  switch (dimension) {
    case 0: base = 2; break;
    case 1: base = 3; break;
    case 2: base = 5; break;
    case 3: base = 7; break;
    case 4: base = 11; break;
    case 5: base = 13; break;
    case 6: base = 17; break;
    case 7: base = 19; break;
    default: base = 2; break;
  }
  // clang-format on

  // calculate radiacal inverse
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
