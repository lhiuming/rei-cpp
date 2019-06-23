// source of camera.h
#include "camera.h"

#include <algorithm>

#include "console.h"
#include "debug.h"

using namespace std;

const double PI = 3.141592653589793238463; // copied from web :P

namespace rei {

// Default constructor
Camera::Camera() {
  update_transforms();
};

// Initialize with pos, dir and up
Camera::Camera(const Vec3& pos, const Vec3& dir, const Vec3& up)
    : up(up.normalized()), position(pos), direction(dir.normalized()) {
  update_transforms();
}

// Configurations //

// update aspect
void Camera::set_aspect(double aspect) {
  this->aspect = aspect;
  update_c2n();
  update_w2n();
}

// update a bunch parameters
void Camera::set_params(double aspect, double angle, double znear, double zfar) {
  this->aspect = aspect;
  this->angle = min(max(angle, 5.0), 160.0);
  this->znear = znear;
  this->zfar = zfar;

  if ((angle < 5.0) || (angle > 160.0)) {
    console << "Camera Warning: unusual angle : " << angle << ", clipped." << endl;
  }

  update_c2n();
  update_w2n();
}

// Dynamics Configurations //

void Camera::zoom(double q) {
  angle = min(max(angle - q, 5.0), min(160.0, 160.0 * aspect));
  update_c2n();
  update_w2n();
}

void Camera::move(double right, double up, double fwd) {
  Vec3 orth_u = this->right();
  Vec3 orth_v = this->up;
  Vec3 orth_w = this->direction;
  position += (right * orth_u + up * orth_v + fwd * orth_w);
  update_w2c();
  update_w2n();
}

void Camera::rotate_position(const Vec3& center, const Vec3& axis, double radian) {
  Vec3 vec_to_me = position - center;
  if (dot(axis, direction) < 0.9995) {
    Vec3::rotate(vec_to_me, axis.normalized(), radian);
    position = center + vec_to_me;
    mark_view_trans_dirty();
  }
}

void Camera::rotate_direction(const Vec3& axis, double radian) {
  Vec3::rotate(direction, axis.normalized(), radian);
  mark_view_trans_dirty();
}

void Camera::look_at(const Vec3& target, const Vec3& up_hint) {
  Vec3 new_dir = target - position;
  if (new_dir.norm2() > 0) {
    direction = new_dir.normalized();
    if (up_hint.norm2() > 0) up = up_hint.normalized();
    mark_view_trans_dirty();
  }
}

// Visibility query
bool Camera::visible(const Vec3& v) const {
  double dist = (position - v).norm();
  return (znear < dist || dist < zfar);
}

// Compute right-hand-world to left-hand-camera row-wise transform
void Camera::update_w2c() {
  // translation_t (column-wise)
  Mat4 translate_t = Mat4::I();
  translate_t[3] = Vec4(-position, 1.0);

  // create a orthogonal coordiante for camera (used in rotation)
  Vec3 orth_u, orth_v, orth_w;
  orth_w = direction.normalized(); // `forward direction`
  Vec3 u = cross(orth_w, up);      // `right direction`
  if (u.zero())
    u = orth_u; // handle the case of bad-direction
  else
    orth_u = u.normalized();
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
void Camera::update_c2n() {
  // NOTE: Compute them as row-major, then transpose before return

  // 1. make the view-pymirad into retangular pillar (divided by z disntance)
  // then replace z by -1/z  (NOTE: assuming h == 1), so further is larger
  static constexpr double p_data[] = // a private constexpr
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0};
  Mat4 P(p_data);

  // 2. move the pillar along z axis, making it center at origin
  Mat4 M = Mat4::I();
  M(2, 3) = (1.0 / zfar + 1.0 / znear) / 2.0;

  // 3. normalize each dimension (x, y, z)
  double pillar_half_width = tan(angle / 2 * (PI / 180.0)); // use radian
  double pillar_half_height = pillar_half_width / aspect;
  double pillar_half_depth = (1.0 / znear - 1.0 / zfar) / 2.0;
  Mat4 C(Vec4(1.0 / pillar_half_width, 1.0 / pillar_half_height, 1.0 / pillar_half_depth, 1.0));

  // composition
  camera2normalized = (C * M * P).T();
}

// Update world2normalized
void Camera::update_w2n() {
  static Mat4 normalized2viewport {
    1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 2.0};
  world2normalized = world2camera * camera2normalized; // they are row-wise
  world2viewport = world2normalized * normalized2viewport;
}

// Debug print
std::wostream& operator<<(std::wostream& os, const Camera& cam) {
  os << "Camrea:" << endl;
  os << "  position : " << cam.position << endl;
  os << "  direction: " << cam.direction << endl;
  os << "  up       : " << cam.up << std::endl;
  os << "  params: "
     << "fov-angle = " << cam.angle << ", aspect = " << cam.aspect << ", z-near = " << cam.znear
     << ", z-fat = " << cam.zfar << endl;

  return os;
}

} // namespace rei
