#ifndef SPHERE_H
#define SPHERE_H

#include <cmath>
#include "geometry/geometry.h"

class sphere : public geometry {
  public:
    sphere(const vec3& center, double radius, int mat_id)
      : geometry(mat_id), center_(center), radius_(std::fabs(radius)) {}

    const vec3& center() const { return center_; }
    double radius() const { return radius_; }

    bool hit(const ray& r, const interval* t_range, intersection& isect) const override;
    vec3 normal(const vec3& point) const override;
    bool sample_emitter_point(vec3& p, vec3& n, double& pdf_area) const override;
    double surface_area() const override;

  private:
    vec3 center_;
    double radius_;
};

inline bool sphere::hit(const ray& r, const interval* t_range, intersection& isect) const {
  if (t_range == nullptr) {
    return false;
  }
  vec3 oc = r.origin() - center_;
  auto a = dot(r.direction(), r.direction());
  auto b = 2.0 * dot(oc, r.direction());
  auto c = dot(oc, oc) - radius_ * radius_;
  auto discriminant = b * b - 4 * a * c;
  if (discriminant < 0) {
    return false;
  }
  const auto sqrt_d = std::sqrt(discriminant);
  auto t = (-b - sqrt_d) / (2.0 * a);
  if (!t_range->surrounds(t)) {
    t = (-b + sqrt_d) / (2.0 * a);
    if (!t_range->surrounds(t)) {
      return false;
    }
  }
  isect.point = r.at(t);
  isect.t = t;
  isect.surface = this;
  isect.mat_id = mat_id_;
  return true;
}

inline vec3 sphere::normal(const vec3& point) const {
  return (point - center_);
}

inline bool sphere::sample_emitter_point(vec3& p, vec3& n, double& pdf_area) const {
  n = random_unit_vector();
  p = center_ + radius_ * n;
  pdf_area = 1.0 / (4.0 * PI * radius_ * radius_);
  return true;
}

inline double sphere::surface_area() const {
  return 4.0 * PI * radius_ * radius_;
}

#endif
