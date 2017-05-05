#ifndef CEL_COLOR_H
#define CEL_COLOR_H

/*
 * color.h
 * Define how color is stored and computed.
 */

namespace CEL {

class Color {
public:

  // Color components. All ranges in [0, 1].
  float r;
  float g;
  float b;
  float a;

  // Default constructor
  Color() = default;

  // Initialize with RGB channels.
  Color(float r, float g, float b) : r(r), g(g), b(b), a(1.0) {};

  // Scalar Multiplication. Useful for interpolation
  Color operator*(float c) { return Color{r * c, g * c, b * c, a * c}; }

};

} // namespace CEL

#endif
