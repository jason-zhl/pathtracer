#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"
#include <algorithm>
#include <iostream>

using color = vec3;

// Simplified ACES Narkowicz implementation
inline vec3 ACESFilm(const vec3& x) {
    const double a = 2.51;
    const double b = 0.03;
    const double c = 2.43;
    const double d = 0.59;
    const double e = 0.14;
    const vec3 y = (x * (a * x + b)) / (x * (c * x + d) + e);
    return vec3(std::clamp(y.x(), 0.0, 1.0), std::clamp(y.y(), 0.0, 1.0),
                std::clamp(y.z(), 0.0, 1.0));
}

static constexpr double exposure = 1.2;

inline void write_color(std::ostream &out, const color& pixel_color) {
  const auto scaled = ACESFilm(pixel_color * exposure);
  out << static_cast<int>(255.999 * scaled.x()) << ' '
      << static_cast<int>(255.999 * scaled.y()) << ' '
      << static_cast<int>(255.999 * scaled.z()) << '\n';
}

#endif