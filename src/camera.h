#ifndef REI_CAMERA_H
#define REI_CAMERA_H

#include "rmath.h"
#include "algebra.h"

/*
 * camera.h
 * Representing a camera looking into -Z axis of a right-hand coordinate (or +Z axis of a left-hand
 * coordinate), with Y axis as upward direction.
 *
 * Internally using a right-handed coordinate for all cached transforms and direction vectors,
 * so no flipping happens in internal computation. Also cached transforms are always targeting
 * column vectors.
 *
 * TODO: add sematic parameters (focus distance, like a real camera)
 * TODO: add depth
 * TODO: support tiltering ?
 */

namespace rei {

class Camera {
public:
  // Default constructor (at origin, looking at -z axis)
  Camera();

  // Initialize with position and direction
  Camera(const Vec3& pos, const Vec3& dir = {0.0, 0.0, -1.0}, const Vec3& up = {0.0, 1.0, 0.0},
    Handness handness = Handness::Right);

  // Configurations
  void set_aspect(double width2height); // widht / height
  void set_aspect(int width, int height) { set_aspect(double(width) / height); }
  void set_params(double aspect, double angle, double znear, double zfar);

  // May-be useful queries
  double aspect() const { return m_aspect; }
  double fov_h() const { return angle; }
  double fov_v() const { return angle / m_aspect; }
  Vec3 position() const { return m_position; }
  Vec3 right() const { return cross(m_direction, m_up); }

  // Dynamic Configurations
  void zoom(double quantity);
  void move(double right, double up, double fwd);
  void rotate_position(const Vec3& center, const Vec3& axis, double radian);
  void rotate_direction(const Vec3& axis, double radian);
  void look_at(const Vec3& target, const Vec3& up_hint = {0, 0, 0});
  void update_rotation(const Vec3& forward, const Vec3& up_hint = {0, 0, 0});

  // Get transforms (result in Left Hand Coordinate;
  Mat4 world_to_camera(Handness from = Handness::Right, Handness to = Handness::Right,
    VectorTarget vec = VectorTarget::Column) const {
    return convert(m_world_to_camera, from, to, vec);
  }
  Mat4 camera_to_device(Handness from = Handness::Right, Handness to = Handness::Right,
    VectorTarget vec = VectorTarget::Column) const {
    return convert(m_camera_to_device, from, to, vec);
  }
  Mat4 world_to_device(Handness from = Handness::Right, Handness to = Handness::Right,
    VectorTarget vec = VectorTarget::Column) const {
    return convert(m_world_to_c_to_device, from, to, vec);
  }
  Mat4 world_to_device_halfz(Handness from = Handness::Right, Handness to = Handness::Right,
    VectorTarget vec = VectorTarget::Column) const {
    return convert(m_world_to_c_to_device_h, from, to, vec);
  }
 
  Mat4 view(Handness from = Handness::Right, Handness to = Handness::Right,
    VectorTarget vec = VectorTarget::Column) const {
    return world_to_camera(from, to, vec);
  }
  Mat4 project(Handness from = Handness::Right, Handness to = Handness::Right,
    VectorTarget vec = VectorTarget::Column) const {
    return camera_to_device(from, to, vec);
  }
  Mat4 view_proj(Handness from = Handness::Right, Handness to = Handness::Right,
    VectorTarget vec = VectorTarget::Column) const {
    return world_to_device(from, to, vec);
  }
  Mat4 view_proj_halfz(Handness from = Handness::Right, Handness to = Handness::Right,
    VectorTarget vec = VectorTarget::Column) const {
    return world_to_device_halfz(from, to, vec);
  }


  // Misc query
  Vec3 bln() const {
    return m_position - right() * (std::tan(angle * degree / 2) * znear) + m_direction * znear - m_up * (std::tan(angle * degree / 2) * znear / m_aspect);
  }

  // Visibility query
  bool visible(const Vec3& v) const;

  // Debug print
  friend std::wostream& operator<<(std::wostream& os, const Camera& cam);

private:
  // Note: position and direction are in right-haneded world space
  Vec3 m_up = Vec3(0.0, 1.0, 0.0);
  Vec3 m_position = Vec3(0.0, 0.0, 0.0);
  Vec3 m_direction = Vec3(0.0, 0.0, -1.0); // looking at -z axis in world
  double angle = 60;                       // horizontal view-angle range, by degree
  double m_aspect = 4.0 / 3.0;             // width / height
  double znear = 1.0, zfar = 1000.0;       // distance of two planes of the frustrum

  Mat4 m_world_to_camera;             // defined by position and direction/up
  Mat4 m_camera_to_device;            //  projection and normalization
  Mat4 m_camera_to_device_h;            //  projection and normalization, but with narrower z-range ([0, 1]) in device space
  Mat4 m_world_to_c_to_device;        // composed from above 2
  Mat4 m_world_to_c_to_device_h;        // composed from above 2, but with narrower z-range ([0, 1]) in device space
  Mat4 m_world_to_c_to_d_to_viewport; // above combined with a static normalized->viewport step

  // helper for interface
  static inline Mat4 convert(const Mat4& mat, Handness from = Handness::Right, Handness to = Handness::Right,
    VectorTarget vec = VectorTarget::Column) {
    return convention_convert(mat, from != Handness::Right, to != Handness::Right, vec != VectorTarget::Column);
  }

  // helpers to update transforms
  void update_world_to_camera();
  void update_camera_to_device();
  void update_world_to_camera_to_device();
  void update_transforms() {
    update_world_to_camera();
    update_camera_to_device();
    update_world_to_camera_to_device();
  }
  void mark_view_trans_dirty() { update_transforms(); }
  void mark_proj_trans_dirty() { update_transforms(); }
};

} // namespace rei

#endif
