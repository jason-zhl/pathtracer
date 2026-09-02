#ifndef VEC3_H
#define VEC3_H

#include "rng.h"

#include <iostream>

class vec3 {
  public:
    float e[3] = {0, 0, 0};

    HOST_DEVICE vec3() : e{0, 0, 0} {}
    HOST_DEVICE vec3(float e0, float e1, float e2) : e{e0, e1, e2} {}

    HOST_DEVICE float x() const { return e[0]; }
    HOST_DEVICE float y() const { return e[1]; }
    HOST_DEVICE float z() const { return e[2]; }

    HOST_DEVICE vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
    HOST_DEVICE float operator[](int i) const { return e[i]; }
    HOST_DEVICE float& operator[](int i) { return e[i]; }

    HOST_DEVICE vec3& operator+=(const vec3 &v) {
      e[0] += v.e[0];
      e[1] += v.e[1];
      e[2] += v.e[2];
      return *this;
    }

    HOST_DEVICE vec3& operator+=(float t) {
      e[0] += t;
      e[1] += t;
      e[2] += t;
      return *this;
    }

    HOST_DEVICE vec3& operator*=(const vec3 &v) {
      e[0] *= v.e[0];
      e[1] *= v.e[1];
      e[2] *= v.e[2];
      return *this;
    }

    HOST_DEVICE vec3& operator*=(float t) {
      e[0] *= t;
      e[1] *= t;
      e[2] *= t;
      return *this;
    }

    HOST_DEVICE vec3& operator/=(float t) {
      return *this *= 1.0f / t;
    }

    HOST_DEVICE float length() const {
      return sqrtf(length_squared());
    }

    HOST_DEVICE float length_squared() const {
      return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
    }
};

// Utility functions
inline std::ostream& operator<<(std::ostream &out, const vec3 &v) {
  return out << v.e[0] << " " << v.e[1] << " " << v.e[2];
}

HOST_DEVICE inline vec3 operator+(const vec3& u, const vec3& v) {
  return vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}

HOST_DEVICE inline vec3 operator+(const vec3& v, float t) {
  return vec3(v.e[0] + t, v.e[1] + t, v.e[2] + t);
}

HOST_DEVICE inline vec3 operator+(float t, const vec3& v) {
  return v + t;
}

HOST_DEVICE inline vec3 operator-(const vec3& u, const vec3& v) {
  return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}

HOST_DEVICE inline vec3 operator*(const vec3& u, const vec3& v) {
  return vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}

HOST_DEVICE inline vec3 operator*(float t, const vec3& v) {
  return vec3(t*v.e[0], t*v.e[1], t*v.e[2]);
}

HOST_DEVICE inline vec3 operator*(const vec3& v, float t) {
  return t * v;
}

HOST_DEVICE inline vec3 operator/(const vec3& v, float t) {
  return (1.0f / t) * v;
}

HOST_DEVICE inline vec3 operator/(const vec3& u, const vec3& v) {
  return vec3(u.e[0] / v.e[0], u.e[1] / v.e[1], u.e[2] / v.e[2]);
}

HOST_DEVICE inline float dot(const vec3& u, const vec3& v) {
  return u.e[0] * v.e[0]
       + u.e[1] * v.e[1]
       + u.e[2] * v.e[2];
}

HOST_DEVICE inline vec3 cross(const vec3& u, const vec3& v) {
  return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
              u.e[2] * v.e[0] - u.e[0] * v.e[2],
              u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

HOST_DEVICE inline vec3 unit_vector(const vec3& v) {
  return v / v.length();
}

// Branchless ONB method, by Duff et al.
HOST_DEVICE inline void orthonormal_basis(const vec3& n, vec3& t, vec3& b) {
  float sign = copysignf(1.0f, n.z());
  const float a = -1.0f / (sign + n.z());
  const float b_val = n.x() * n.y() * a;
  t = vec3(1.0f + sign * n.x() * n.x() * a, sign * b_val, -sign * n.x());
  b = vec3(sign * b_val, sign + n.y() * n.y() * a, -n.y());
}

HOST_DEVICE inline vec3 random_unit_vector(RNG& rng) {
  for (;;) {
    vec3 p(rng.next() * 2.0f - 1.0f, rng.next() * 2.0f - 1.0f, rng.next() * 2.0f - 1.0f);
    const float len2 = p.length_squared();
    if (len2 <= 1.0f && len2 > 1e-20f) {
      return p / sqrtf(len2);
    }
  }
}

HOST_DEVICE inline vec3 random_in_unit_disk(RNG& rng) {
  const float r = sqrtf(rng.next());
  const float theta = 2.0f * PI * rng.next();
  return vec3(r * cosf(theta), r * sinf(theta), 0.0f);
}

// Cosine-weighted importance sampling for diffuse surfaces
HOST_DEVICE inline vec3 lambertian_random(const vec3& n, RNG& rng) {
  float u = rng.next();
  float v = rng.next();
  float phi = 2.0f * PI * u;
  float r = sqrtf(v);
  float x = r * cosf(phi);
  float y = r * sinf(phi);
  float z = sqrtf(1.0f - v);
  vec3 t, b;
  orthonormal_basis(n, t, b);
  return t * x + b * y + n * z;
}

#endif
