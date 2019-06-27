// source of camera.h
#include "camera.h"

#include <algorithm>

#include "console.h"
#include "debug.h"
#include "rmath.h"

// using namespace std;

namespace rei {

// Default constructor
Camera::Camera() {
  update_transforms();
};

// Initialize with pos, dir and up
Camera::Camera(const Vec3& pos, const Vec3& dir, const Vec3& up, Handness handness)
    : m_up(up.normalized()), m_position(pos), m_direction(dir.normalized()) {
  if (handness == Handness::Left) {
    flip_z(m_up);
    flip_z(m_position);
    flip_z(m_direction);
  }
  update_transforms();
}

// Configurations //

// update aspect
void Camera::set_aspect(double aspect) {
  this->m_aspect = aspect;
  update_camera_to_device();
  update_world_to_camera_to_device();
}

// update a bunch parameters
void Camera::set_params(double aspect, double angle, double znear, double zfar) {
  REI_ASSERT((0 < znear) && (znear < zfar));
  if ((angle < 5.0) || (angle > 160.0)) {
    console << "Camera Warning: unusual angle : " << angle << ", clipped." << endl;
  }

  this->m_aspect = aspect;
  this->angle = min(max(angle, 5.0), 160.0);
  this->znear = znear;
  this->zfar = zfar;

  mark_proj_trans_dirty();
}

// Dynamics Configurations //

void Camera::zoom(double q) {
  angle = min(max(angle - q, 5.0), min(160.0, 160.0 * m_aspect));
  mark_proj_trans_dirty();
}

void Camera::move(double right, double up, double fwd) {
  m_position += (right * this->right() + up * this->m_up + fwd * this->m_direction);
  mark_view_trans_dirty();
}

void Camera::rotate_position(const Vec3& center, const Vec3& axis, double radian) {
  if (dot(axis, m_direction) > 0.9995) return;

  Vec3 vec_to_me = m_position - center;
  Vec3::rotate(vec_to_me, axis.normalized(), radian);
  m_position = center + vec_to_me;

  mark_view_trans_dirty();
}

void Camera::rotate_direction(const Vec3& axis, double radian) {
  Vec3::rotate(m_direction, axis.normalized(), radian);
  mark_view_trans_dirty();
}

void Camera::look_at(const Vec3& target, const Vec3& up_hint) {
  Vec3 new_dir = target - m_position;
  update_rotation(new_dir, up_hint);
}

void Camera::update_rotation(const Vec3& forward, const Vec3& up_hint) {
  if (forward.norm2() >= 0.0001) {
    m_direction = forward.normalized();
    if (up_hint.norm2() >= 0.01) m_up = up_hint.normalized();
    // Ensure orthogonality
    m_up = m_up - dot(m_up, m_direction) * m_direction;
    Vec3::normalize(m_up);
  } else {
    REI_WARNING("input forward is in bad form (norm < 0.01)");
  }
  mark_view_trans_dirty();
}

// Visibility query
bool Camera::visible(const Vec3& v) const {
  // TODO check the frestrum
  double dist = (m_position - v).norm();
  return (znear < dist || dist < zfar);
}

// Compute the world-space camera-space (or view-space) transform
void Camera::update_world_to_camera() {
  // NOTE: translate first, than rotate

  // translation
  Mat4 T = Mat4::translate(-m_position);

  // orthogonal coordiante for camera space (as rotation)
  Vec3 orth_w = -m_direction; // `forward direction` looking into -Z axis
  Vec3 orth_v = m_up;
  Vec3 orth_u = cross(orth_v, orth_w);
  Mat4 R {{orth_u, 0}, {orth_v, 0}, {orth_w, 0}, {{}, 1}};
  Mat4::transpose(R);

  // now, translate is affect by rotation
  m_world_to_camera = R * T;
}

// Naive computation of camera-space-to-normalized-space (Normalized Device Coordinate) transform
// Applying camera projection and normalization.
void Camera::update_camera_to_device() {
  // NOTE: Compute them as row-major, then transpose before return

  // 1. make the view-frustrum into retangular pillar (divided by -z)
  // then replace z by -1/z  (NOTE: assuming h == 1), so further is smaller (keeping z order)
  constexpr Mat4 P {
    1, 0, 0, 0,  //
    0, 1, 0, 0,  //
    0, 0, 0, 1,  //
    0, 0, -1, 0, //
  };

  // 2. move the pillar along -z axis, making the frustrum it center at origin
  Mat4 M = Mat4::I();
  M(2, 3) = -(1.0 / zfar + 1.0 / znear) / 2.0;

  // 3. normalize each dimension (x, y, z)
  double pillar_half_width = tan(angle * (0.5 * degree)); // use radian
  double pillar_half_height = pillar_half_width / m_aspect;
  double pillar_half_depth = (1.0 / znear - 1.0 / zfar) / 2.0;
  Mat4 C = Mat4::from_diag(Vec4(1.0 / pillar_half_width, 1.0 / pillar_half_height, 1.0 / pillar_half_depth, 1.0));

  // composition
  m_camera_to_device = C * M * P;
}

// Compose and cache them
void Camera::update_world_to_camera_to_device() {
  // NOTE: matrix are targeting column vectors
  m_world_to_c_to_device = m_camera_to_device * m_world_to_camera;
  constexpr Mat4 device_to_viewport {
    1.0, 0.0, 0.0, 1.0, //
    0.0, 1.0, 0.0, 1.0, //
    0.0, 0.0, 1.0, 1.0, //
    0.0, 0.0, 0.0, 2.0, //
  };
  m_world_to_c_to_d_to_viewport = device_to_viewport * m_world_to_c_to_device;
}

// Debug print
std::wostream& operator<<(std::wostream& os, const Camera& cam) {
  os << "Camrea:" << endl;
  os << "  position : " << cam.m_position << endl;
  os << "  direction: " << cam.m_direction << endl;
  os << "  up       : " << cam.m_up << std::endl;
  os << "  params: "
     << "fov-angle = " << cam.angle << ", aspect = " << cam.m_aspect << ", z-near = " << cam.znear
     << ", z-fat = " << cam.zfar << endl;

  return os;
}

} // namespace rei
