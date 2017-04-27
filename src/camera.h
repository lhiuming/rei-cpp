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

  // default constructor
  Camera();

  // contruct with given position and direction
  Camera(Vec3 pos, Vec3 dir);

  // configuration
  void set_position(Vec3 pos);
  void set_target(Vec3 target);
  void set_direction(Vec3 dir);
  void set_angle(double theta); // vertical view range
  void set_ratio(double ratio); // widht / height

private:

  void update_transform(); // make the up-right coordination

  Vec3 position = Vec3(0.0, 0.0, 0.0);
  Vec3 direction = Vec3(0.0, 0.0, -1); // looking at -z axis
  Mat4 world2camera;
  Mat4 camera2normalized;

};

} // namespace CEL

#endif
