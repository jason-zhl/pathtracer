#ifndef PLANE_PATCH_H
#define PLANE_PATCH_H

#include <cmath>
#include "geometry.h"

// Finite planar patch: corner + s*u + t*v for s,t in [0,1] (parallelogram).
class plane_patch : public geometry {
  public:
    plane_patch(const vec3& corner, const vec3& u, const vec3& v)
        : corner_(corner), u_(u), v_(v), n_(cross(u, v)), n_len_sq_(n_.length_squared()) {}

    bool hit(const ray& r, intersection& isect) const override;
    vec3 normal(const vec3& point) const override;

  private:
    vec3 corner_;
    vec3 u_;
    vec3 v_;
    vec3 n_;
    double n_len_sq_;
};

inline bool plane_patch::hit(const ray& r, intersection& isect) const {
  if (n_len_sq_ < 1e-30) {
    return false;
  }
  double denom = dot(r.direction(), n_);
  if (std::fabs(denom) < 1e-12) {
    return false;
  }
  double t = dot(corner_ - r.origin(), n_) / denom;
  if (t < 0) {
    return false;
  }
  vec3 p = r.at(t);
  vec3 w = p - corner_;
  double s = dot(cross(w, v_), n_) / n_len_sq_;
  double tv = dot(cross(u_, w), n_) / n_len_sq_;
  if (s < 0.0 || s > 1.0 || tv < 0.0 || tv > 1.0) {
    return false;
  }
  isect.point = p;
  isect.t = t;
  isect.surface = this;
  return true;
}

inline vec3 plane_patch::normal(const vec3& point) const {
  (void)point;
  return n_;
}

#endif
