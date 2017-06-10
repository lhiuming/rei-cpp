// source of camera.h
#include "camera.h"

#include <iostream> // debug
#include <algorithm>

using namespace std;

const double PI = 3.141592653589793238463; // copied from web

namespace CEL {

// Default constructor
Camera::Camera()
{
  update_transforms();
};

// Initialize with pos and dir
Camera::Camera(const Vec3& pos) : position(pos)
{
  update_transforms();
}
Camera::Camera(const Vec3& pos, const Vec3& dir)
 : position(pos), direction(dir)
{
  update_transforms();
}


// Configurations //

void Camera::set_target(const Vec3& target)
{
  direction = target - position;
  update_transforms();
}

// update ratio
void Camera::set_ratio(double ratio)
{
  this->ratio = ratio;
  update_c2n();
  update_w2n();
}


// Dynamics Configurations //

void Camera::zoom(double q)
{
  angle = min(max(angle - q, 5.0), min(160.0, 160.0 * ratio));
  update_c2n();
  update_w2n();
}

void Camera::move(double right, double up, double fwd)
{
  position += (right * orth_u + up * orth_v + fwd * orth_w);
  update_w2c();
  update_w2n();
}

// Visibility query
bool Camera::visible(const Vec3& v) const
{
  double dist = (position - v).norm();
  return ( znear < dist || dist < zfar);
}

// Compute right-hand-world to left-hand-camera row-wise transform
void Camera::update_w2c()
{
  // translation_t (column-wise)
  Mat4 translate_t = Mat4::I();
  translate_t[3] = Vec4(-position, 1.0);

  // create a orthogonal coordiante for camera (used in rotation)
  orth_w = direction.normalized(); // `forward direction`
  Vec3 u = cross(orth_w, up); // `right direction`
  if (u.zero()) u = orth_u; // handle the case of bad-direction
  else orth_u = u.normalized();
  orth_v = cross(u, orth_w).normalized(); // `up direction`

  // rotation (row-wise)
  Mat4 rotate = Mat4::I();
  rotate[0] = Vec4(orth_u, 0.0);
  rotate[1] = Vec4(orth_v, 0.0);
  rotate[2] = Vec4(orth_w, 0.0);

  // compose and update
  world2camera = translate_t.T() * rotate;
}

// Naive computation of left-hand-camera to left-hand-normalized transform
void Camera::update_c2n()
{
  // NOTE: Compute them as row-major, then transpose before return

  // 1. make the view-pymirad into retangular pillar (divided by z disntance)
  // then replace z by -1/z  (NOTE: assuming h == 1), so further is larger
  static constexpr double p_data[] = // a private constexpr
    {1,  0,  0,  0,
     0,  1,  0,  0,
     0,  0,  0, -1,
     0,  0,  1,  0};
  Mat4 P(p_data);

  // 2. move the pillar along z axis, making it center at origin
  Mat4 M = Mat4::I();
  M(2, 3) = (1.0 / zfar + 1.0 / znear) / 2.0;

  // 3. normalize each dimension (x, y, z)
  double pillar_half_width = tan(angle / 2 * (PI / 180.0)); // use radian
  double pillar_half_height = pillar_half_width / ratio;
  double pillar_half_depth = (1.0 / znear - 1.0 / zfar) / 2.0;
  Mat4 C(Vec4(1.0 / pillar_half_width, 1.0 / pillar_half_height,
              1.0 / pillar_half_depth, 1.0));

  // composition
  camera2normalized = (C * M * P).T();
}

// Update world2normalized
void Camera::update_w2n()
{
  static Mat4 normalized2viewport{
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    1.0, 1.0, 1.0, 2.0
  };
  world2normalized = world2camera * camera2normalized; // they are row-wise
  world2viewport = world2normalized * normalized2viewport;
}

} // namespace CEL
