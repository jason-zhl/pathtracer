#ifndef VEC3_H
#define VEC3_H

#include <cmath>
#include <iostream>

class vec3 {
  public:
    double e[3] = {0, 0, 0};

    vec3() = default;
    vec3(double e0, double e1, double e2) : e{e0, e1, e2} {}

    double x() const { return e[0]; }
    double y() const { return e[1]; }
    double z() const { return e[2]; }

    vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
    double operator[](int i) const { return e[i]; }
    double& operator[](int i) { return e[i]; }

    vec3& operator+=(const vec3 &v) {
      e[0] += v.e[0];
      e[1] += v.e[1];
      e[2] += v.e[2];
      return *this;
    }

    vec3& operator+=(double t) {
      e[0] += t;
      e[1] += t;
      e[2] += t;
      return *this;
    }

    vec3& operator*=(double t) {
      e[0] *= t;
      e[1] *= t;
      e[2] *= t;
      return *this;
    }

    vec3& operator/=(double t) {
      return *this *= 1/t;
    }

    double length() const {
      return std::sqrt(length_squared());
    }

    double length_squared() const {
      return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
    }
};

// Utility functions
inline std::ostream& operator<<(std::ostream &out, const vec3 &v) {
  return out << v.e[0] << " " << v.e[1] << " " << v.e[2];
}

inline vec3 operator+(const vec3& u, const vec3& v) {
  return vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}

inline vec3 operator+(const vec3& v, double t) {
  return vec3(v.e[0] + t, v.e[1] + t, v.e[2] + t);
}

inline vec3 operator+(double t, const vec3& v) {
  return v + t;
}

inline vec3 operator-(const vec3& u, const vec3& v) {
  return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}

inline vec3 operator*(const vec3& u, const vec3& v) {
  return vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}

inline vec3 operator*(double t, const vec3& v) {
  return vec3(t*v.e[0], t*v.e[1], t*v.e[2]);
}

inline vec3 operator*(const vec3& v, double t) {
  return t * v;
}

inline vec3 operator/(const vec3& v, double t) {
  return (1/t) * v;
}

inline vec3 operator/(const vec3& u, const vec3& v) {
  return vec3(u.e[0] / v.e[0], u.e[1] / v.e[1], u.e[2] / v.e[2]);
}

inline double dot(const vec3& u, const vec3& v) {
  return u.e[0] * v.e[0]
       + u.e[1] * v.e[1]
       + u.e[2] * v.e[2];
}

inline vec3 cross(const vec3& u, const vec3& v) {
  return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
              u.e[2] * v.e[0] - u.e[0] * v.e[2],
              u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

inline vec3 unit_vector(const vec3& v) {
  return v / v.length();
}

// Branchless ONB method, by Duff et al.
inline void orthonormal_basis(const vec3& n, vec3& t, vec3& b) {
  double sign = std::copysign(1.0, n.z());
  const double a = -1.0 / (sign + n.z());
  const double b_val = n.x() * n.y() * a;
  t = vec3(1.0 + sign * n.x() * n.x() * a, sign * b_val, -sign * n.x());
  b = vec3(sign * b_val, sign + n.y() * n.y() * a, -n.y());
}

inline vec3 random_unit_vector() {
  for (;;) {
    vec3 p(random_double() * 2.0 - 1.0, random_double() * 2.0 - 1.0, random_double() * 2.0 - 1.0);
    const double len2 = p.length_squared();
    if (len2 <= 1.0 && len2 > 1e-20) {
      return p / std::sqrt(len2);
    }
  }
}

// Cosine-weighted importance sampling for diffuse surfaces
inline vec3 lambertian_random(const vec3& n) {
  double u = random_double();
  double v = random_double();
  double phi = 2 * PI * u;
  double r = std::sqrt(v);
  double x = r * std::cos(phi);
  double y = r * std::sin(phi);
  double z = std::sqrt(1.0 - v);
  vec3 t, b;
  orthonormal_basis(n, t, b);
  return t * x + b * y + n * z;
}

#endif
