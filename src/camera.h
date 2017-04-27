#ifndef CEL_CAMERA_H
#define CEL_CAMERA_H

#include "algebra.h"

/*
 * camera.h
 * Define the Camera class, used to created a viewport and provide world-to-
 * camera transformation.
 *
 * TODO: add sematic parameters
 * TODO: add depth
 */

namespace CEL {

class Camera {

public:

  // Default constructor (at origin, looking at -z axis)
  Camera();

  // Initialize with position and direction
  Camera(Vec3 pos, Vec3 dir);

  // Configurations
  void set_position(Vec3 pos);
  void set_target(Vec3 target);
  void set_direction(Vec3 dir);
  void set_angle(double theta); // vertical view range
  void set_ratio(double ratio); // widht / height

  // Get transforms
  const Mat4& get_w2c() { return world2camera; }
  const Mat4& get_c2n() { return camera2normalized; }
  const Mat4& get_w2n() { return world2normalized; }

private:

  Vec3 position = Vec3(0.0, 0.0, 0.0);
  Vec3 direction = Vec3(0.0, 0.0, -1.0); // looking at -z axis
  double angle = 60;             // view-angle range, by degree
  double ratio = 4.0 / 3.0;      // width / height
  double near = 5.0, far = 1000; // distance of two planes of the frustrum

  Mat4 world2camera;       // defined by position and direction
  Mat4 camera2normalized;  // defined by view angle and ration

  Mat4 world2normalized;   // composed from above 2

  // helpers to update transforms
  Vec3 u_hint = Vec3(1.0, 0, 0); // used when camera direction is up
  void update_w2c();
  void update_c2n();
  void update_w2n();

};

} // namespace CEL

#endif
