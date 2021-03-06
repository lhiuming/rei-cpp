// Test the camera modeule
#include <cmath>
#include <iostream>
#include <vector>

#include <camera.h>
#include <console.h>

using namespace CEL;
using namespace std;

int main() {
  console << "--- Check default camera ---" << endl;
  Camera c;
  double def_ratio = 4.0 / 3.0;
  double half_w = 5 * tan(3.14159265 / 6), half_h = half_w / def_ratio;
  double half_w_f = 1000 * tan(3.14159265 / 6), half_h_f = half_w_f / def_ratio;
  vector<Vec4> world_points;
  world_points.push_back(Vec4(half_w, half_h, -5, 1)); // front face
  world_points.push_back(Vec4(-half_w, half_h, -5, 1));
  world_points.push_back(Vec4(-half_w, -half_h, -5, 1));
  world_points.push_back(Vec4(half_w, -half_h, -5, 1));
  world_points.push_back(Vec4(half_w_f, half_h_f, -1000, 1)); // back face
  world_points.push_back(Vec4(-half_w_f, half_h_f, -1000, 1));
  world_points.push_back(Vec4(-half_w_f, -half_h_f, -1000, 1));
  world_points.push_back(Vec4(half_w_f, -half_h_f, -1000, 1));

  console << ">> World points: " << endl;
  for (unsigned i = 0; i < world_points.size(); ++i)
    console << "x" << i << " = " << world_points[i] << endl;

  console << ">> Camera points: " << endl;
  const Mat4& w2c = c.get_w2c();
  console << "w2c = " << w2c << endl;
  for (unsigned i = 0; i < world_points.size(); ++i)
    console << "x * w2c " << i << " = " << world_points[i] * w2c << endl;

  console << ">> Normalized points: " << endl;
  console << "c2n = " << c.get_c2n() << endl;
  const Mat4& w2n = c.get_w2n();
  console << "w2n = " << w2n << endl;
  for (unsigned i = 0; i < world_points.size(); ++i)
    console << "x * w2n " << i << " = " << Vec3(world_points[i] * w2n) << endl;

  console << ">> Viewport points: " << endl;
  const Mat4& w2v = c.get_w2v();
  console << "w2v = " << w2v << endl;
  for (unsigned i = 0; i < world_points.size(); ++i)
    console << "x * w2v " << i << " = " << Vec3(world_points[i] * w2v) << endl;

  return 0;
}
