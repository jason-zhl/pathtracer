#ifndef PLANE_PATCH_H
#define PLANE_PATCH_H

#include <cmath>

inline bool plane_hit(const Geometry& g, const ray& r, const interval* t_range,
  intersection& isect) {
  if (t_range == nullptr) {
    return false;
  }
  if (g.n_len_sq < 1e-30) {
    return false;
  }
  double denom = dot(r.direction(), g.n);
  if (std::fabs(denom) < 1e-12) {
    return false;
  }
  double t = dot(g.corner - r.origin(), g.n) / denom;
  if (!t_range->surrounds(t)) {
    return false;
  }
  vec3 p = r.at(t);
  vec3 w = p - g.corner;
  double s = dot(cross(w, g.v), g.n) / g.n_len_sq;
  double tv = dot(cross(g.u, w), g.n) / g.n_len_sq;
  if (s < 0.0 || s > 1.0 || tv < 0.0 || tv > 1.0) {
    return false;
  }
  isect.point = p;
  isect.t = t;
  isect.normal = g.n;
  isect.mat_id = g.mat_id;
  return true;
}

inline vec3 plane_normal(const Geometry& g, const vec3& point) {
  (void)point;
  return g.n;
}

inline bool plane_sample_emitter_point(const Geometry& g, vec3& p, vec3& n, double& pdf_area) {
  const double su = random_double();
  const double sv = random_double();
  p = g.corner + su * g.u + sv * g.v;
  n = unit_vector(g.n);
  const double a = std::sqrt(g.n_len_sq);
  if (a < 1e-30) {
    return false;
  }
  pdf_area = 1.0 / a;
  return true;
}

inline double plane_surface_area(const Geometry& g) {
  return std::sqrt(g.n_len_sq);
}

#endif
