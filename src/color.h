#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"
#include <iostream>

using color = vec3;

// Simplified ACES Narkowicz implementation
inline vec3 ACESFilm(const vec3& x) {
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    const vec3 y = (x * (a * x + b)) / (x * (c * x + d) + e);
    return vec3(clamp(y.x(), 0.0f, 1.0f), clamp(y.y(), 0.0f, 1.0f),
                clamp(y.z(), 0.0f, 1.0f));
}

// static constexpr float exposure = 1.2;

inline void write_color(std::ostream &out, const color& pixel_color) {
  // const auto scaled = ACESFilm(pixel_color * exposure);
  const auto scaled = ACESFilm(pixel_color);
  out << static_cast<int>(255.999f * scaled.x()) << ' '
      << static_cast<int>(255.999f * scaled.y()) << ' '
      << static_cast<int>(255.999f * scaled.z()) << '\n';
}

#endif