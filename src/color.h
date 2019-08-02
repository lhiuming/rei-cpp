#ifndef REI_COLOR_H
#define REI_COLOR_H

#include <cstdint>
#include <ostream>

#include "algebra.h"

/*
 * color.h
 * Define how color is stored and computed.
 */

namespace rei {

class Color {
  using byte = std::uint8_t;

public:
  // Color components. All ranges in [0, 1].
  float r = 0.f;
  float g = 0.f;
  float b = 0.f;
  float a = 0.f;

  // Default constructor
  constexpr Color() = default;

  // Initialize with RGBA or RGBA channels.
  constexpr Color(float r, float g, float b, float a = 1.0) : r(r), g(g), b(b), a(a) {};
  // Initialize RGB by integer in range [0, 255]
  constexpr Color(byte r, byte g, byte b) : r(r / 255.0f), g(g / 255.0f), b(b / 255.0f), a(1.0f) {};
  constexpr Color(int r, int g, int b) : Color(byte(r), byte(g), byte(b)) {}
  // Initialize RGB by hex code
  constexpr Color(int hex) : Color((hex >> 4) & 0xFF, (hex >> 2) & 0xFF, hex & 0xFF) {}

  // Scalar Multiplication. Useful for interpolation
  Color operator*(float c) const { return Color(r * c, g * c, b * c, a * c); }

  // Convert to Vec4
  explicit operator Vec4() const { return {r, g, b, a}; }
};

// Print out the color
std::wostream& operator<<(std::wostream& os, const Color& col);

namespace Colors {

// Some userfull colors

constexpr Color black = {0.0f, 0.0f, 0.0f};
constexpr Color white = {1.0f, 1.0f, 1.0f};
constexpr Color red = {1.0f, 0.0f, 0.0f};
constexpr Color green = {0.0f, 1.0f, 0.0f};
constexpr Color blue = {0.0f, 0.0f, 1.0f};
constexpr Color yellow = {1.0f, 1.0f, 0.0f};
constexpr Color magenta = {1.0f, 0.0f, 1.0f};
constexpr Color aqua = {0.0f, 1.0f, 1.0f};

constexpr Color ayanami_blue = {129, 187, 235};
constexpr Color asuka_red = {156, 0, 0};
constexpr Color jo = {185, 44, 37};
constexpr Color ha = {251, 88, 31};
constexpr Color kyu = {34, 166, 191};
constexpr Color final = {255, 255, 255};

namespace fresnel0 {

// Some PBR reference values
// ref: <Real-Time Rendering> 4th edition, page 323.
constexpr Color titanium = {.542f, .497f, .449f};
constexpr Color chromium = {.549f, .556f, .554f};
constexpr Color iron = {.562f, .565f, .578f};
// constexpr Color nickel;
// constexpr Color platinum;
constexpr Color copper = {.955f, .638f, .538f};
// constexpr Color palladium;
// constexpr Color mercury;
constexpr Color brass_c260 = {.910f, .778f, .423f};
constexpr Color zinc = {.664f, .824f, .850f};
constexpr Color gold = {1.000f, .782f, .344f};
constexpr Color aluminum = {.913f, .922f, .924};
constexpr Color silver = {.972f, .960f, .915f};

} // namespace fresnel0

} // namespace Colors

} // namespace rei

#endif
