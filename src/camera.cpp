// source of camera.h
#include "camera.h"

#include <iostream> // debug
#include <cmath>

using namespace std;

const double PI = 3.141592653589793238463; // copied from web

namespace CEL {

// Default constructor
Camera::Camera() {
  update_w2c();
  update_c2n();
  update_w2n();
};

// Configurations //
// TODO: set them


// Dynamics Configurations //

void Camera::zoom(double q)
{
  angle = min(max(angle - q, 0.0), 180.0);
  update_c2n();
  update_w2n();
}

// Compute world to camera transform
void Camera::update_w2c()
{
  // translation
  Mat4 translate = Mat4::I();
  translate[3] = Vec4(-position, 1.0);

  // create a orthogonal coordiante for the camera
  Vec3 up(0.0, 1.0, 0.0);
  Vec3 u = cross(direction, up);   // right direction
  if (u.zero()) u = u_hint;
  else u_hint = u;
  Vec3 v = cross(u, direction);    // up direction
  Vec3 w = -direction;             // back direction; camera look toward -w

  // rotation
  Mat4 rotate = Mat4::I();
  rotate[0] = Vec4(u.normalized(), 0.0);
  rotate[1] = Vec4(v.normalized(), 0.0);
  rotate[2] = Vec4(w.normalized(), 0.0);
  Mat4::transpose(rotate);

  // compose and update
  world2camera =  rotate * translate;
}

// Naive computation of camera to world
// TODO: improve this
void Camera::update_c2n()
{
  // 1. make the view-pymirad into retangular pillar (divided by -z)
  // as well as replace z by 1/-z (assuming h == 1)
  static double p_data[] = // a private constexpr
    {1,  0,  0,  0,
     0,  1,  0,  0,
     0,  0,  0,  1,
     0,  0, -1,  0};
  Mat4 P(p_data);

  // 2. move the pillar along z axis, making it center at origin
  Mat4 M = Mat4::I();
  M(2, 3) = - (1.0 / far + 1.0 / near) / 2;

  // 3. normalize each dimension (x, y, z)
  double pillar_half_width = tan(angle / 2 * (PI / 180.0)); // use radian
  double pillar_half_height = pillar_half_width / ratio;
  double pillar_half_length = (1.0 / near - 1.0 / far) / 2;
  Mat4 C(Vec4(1.0 / pillar_half_width, 1.0 / pillar_half_height,
              1.0 / pillar_half_length, 1.0));

  // composition
  camera2normalized = C * M * P;
}

// Update world2normalized
void Camera::update_w2n()
{
  world2normalized = camera2normalized * world2camera; // yes, reversed order
}

} // namespace CEL
