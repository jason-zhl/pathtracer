#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"
#include <iostream>

using color = vec3;

// Simplified ACES Narkowicz implementation
HOST_DEVICE inline vec3 ACESFilm(const vec3& x) {
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    const vec3 y = (x * (a * x + b)) / (x * (c * x + d) + e);
    return vec3(clamp(y.x(), 0.0f, 1.0f), clamp(y.y(), 0.0f, 1.0f),
                clamp(y.z(), 0.0f, 1.0f));
}

HOST_DEVICE inline vec3 gamma_filter(const vec3& x) {
    constexpr float inv_gamma = 1.0f / 2.2f;
    return vec3(powf(x.x(), inv_gamma), powf(x.y(), inv_gamma),
                powf(x.z(), inv_gamma));
}

inline void write_color(std::ostream &out, const color& pixel_color) {
  out << static_cast<int>(255.999f * pixel_color.x()) << ' '
      << static_cast<int>(255.999f * pixel_color.y()) << ' '
      << static_cast<int>(255.999f * pixel_color.z()) << '\n';
}

#endif
