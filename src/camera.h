#ifndef CEL_CAMERA_H
#define CEL_CAMERA_H

#include "algebra.h"

/*
 * camera.h
 * Define the Camera class, used to created a viewport and provide world-to-
 * camera transformation. Use Left-Hand coordinate, used to transform 
 * row-vector in Right-Handed world space.
 *
 * TODO: add sematic parameters (focus distance, like a real camera)
 * TODO: add depth
 * TODO: support tiltering ?
 */

namespace CEL {

class Camera {

public:

  // Default constructor (at origin, looking at -z axis)
  Camera();

  // Initialize with position and direction
  Camera(const Vec3& pos);
  Camera(const Vec3& pos, const Vec3& dir);

  // Configurations
  void set_target(const Vec3& target);
  void set_direction(const Vec3& dir);
  void set_angle(double theta); // vertical view range
  void set_ratio(double ratio); // widht / height

  // Dynamic Configurations
  void zoom(double quantity);
  void move(double right, double up, double fwd);

  // Get transforms (result in Left Hand Coordinate; 
  // used to transform row-vector)
  const Mat4& get_w2c() const { return world2camera; }
  const Mat4& get_c2n() const { return camera2normalized; }
  const Mat4& get_w2n() const { return world2normalized; }
  const Mat4& get_w2v() const { return world2viewport;  }

  // Visibility query
  bool visible(const Vec3& v) const;

private:

  // Note: position and direction are in right-haneded world space 
  Vec3 up = Vec3(0.0, 1.0, 0.0);
  Vec3 position = Vec3(0.0, 0.0, 0.0);
  Vec3 direction = Vec3(0.0, 0.0, -1.0); // looking at -z axis in world
  double angle = 60;             // horizontal view-angle range, by degree
  double ratio = 4.0 / 3.0;      // width / height
  double znear = 5.0, zfar = 1000.0; // distance of two planes of the frustrum

  Mat4 world2camera;      // defined by position and direction
  Mat4 camera2normalized; // defined by view angle and ration
  Mat4 world2normalized;  // composed from above 2
  Mat4 world2viewport;    // added a static normalized->viewport step

  // helpers to update transforms
  Vec3 orth_u, orth_v, orth_w;  // unit bases in the world-space; 
  void update_w2c();
  void update_c2n();
  void update_w2n();
  void update_transforms() { update_w2c(); update_c2n(); update_w2n(); }
};

} // namespace CEL

#endif
