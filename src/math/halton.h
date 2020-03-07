#pragma once

#include "algebra.h"

namespace rei {

struct HaltonSequence {
  size_t index = 0;

  HaltonSequence(size_t init_index = 0) : index(init_index) {}

  template <int Base>
  static float sample(int index) {
    float a = 0;
    float inv_base = 1.0f / float(Base);
    for (float mult = inv_base; index != 0; index /= Base, mult *= inv_base) {
      a += float(index % Base) * mult;
    }
    return a;
  }

  float next() { return sample<2>(++index); }
  Vec4 next4() {
    ++index;
    return Vec4(sample<2>(index), sample<3>(index), sample<5>(index), sample<7>(index));
  }
};


}
