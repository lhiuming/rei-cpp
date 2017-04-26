// source of camera.h
#include "camera.h"

// a private constexpr

static double p_data[] =
  {1, 0, 0, 0,
   0, 1, 0, 0,
   0, 0, 1, 0,
   0, 0, 1, 0};

static CEL::Mat4 P(p_data);

namespace CEL {

// camear constructors

// default
Camera::Camera() : camera2normalized(P) {};

// provide position and direction
Camera::Camera(Vec3 pos, Vec3 dir)
 : position(pos), direction(dir), camera2normalized(P)
{
  update_transform();
}

void Camera::update_transform()
{
  // make camera coordinates
  const Vec3 up(0.0, 1.0, 0.0);
  Vec3 u = cross(up, direction);
  if (u.zero()) u = Vec3(1.0, 0.0, 0.0);
  Vec3 v = direction;
  Vec3 w = cross(u, v).normalized();

  // update world to camera transform
  world2camera[0] = Vec4(u, 0);
  world2camera[1] = Vec4(v, 0);
  world2camera[2] = Vec4(w, 0);
  world2camera[3] = Vec4(-position, 1.0);
}


} // namespace CEL
