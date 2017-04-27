// Test the camera modeule
#include <iostream>
#include <vector>
#include <cmath>

#include <camera.h>

using namespace CEL;
using namespace std;

int main() {

  cout << "--- Check default camera ---" << endl;
  Camera c;
  double def_ratio = 4.0 / 3.0;
  double half_w = 5 * tan(3.14159265 / 6), half_h = half_w / def_ratio;
  double half_w_f = 1000 * tan(3.14159265 / 6), half_h_f = half_w_f / def_ratio;
  vector<Vec4> world_points;
  world_points.push_back(Vec4( half_w,  half_h, -5, 1));
  world_points.push_back(Vec4(-half_w,  half_h, -5, 1));
  world_points.push_back(Vec4(-half_w, -half_h, -5, 1));
  world_points.push_back(Vec4( half_w, -half_h, -5, 1));
  world_points.push_back(Vec4( half_w_f,  half_h_f, -1000, 1));
  world_points.push_back(Vec4(-half_w_f,  half_h_f, -1000, 1));
  world_points.push_back(Vec4(-half_w_f, -half_h_f, -1000, 1));
  world_points.push_back(Vec4( half_w_f, -half_h_f, -1000, 1));

  cout << ">> World points: " << endl;
  for (int i = 0; i < world_points.size(); ++i)
    cout << "x" << i << " = " << world_points[i] << endl;

  cout << ">> Camera points: " << endl;
  const Mat4& w2c = c.get_w2c();
  //cout << "w2c = " << w2c << endl;
  for (int i = 0; i < world_points.size(); ++i)
    cout << "w2c * x" << i << " = " << w2c * world_points[i] << endl;

  cout << ">> Normalized points: " << endl;
  const Mat4& w2n = c.get_w2n();
  cout << "w2n = " << w2n << endl;
  for (int i = 0; i < world_points.size(); ++i)
    cout << "w2n * x" << i << " = " << w2n * world_points[i] << endl;

  return 0;
}
