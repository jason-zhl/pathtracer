#ifndef SPHERE_H
#define SPHERE_H

#include <cmath>
#include "geometry.h"
#include "material/material.h"

class sphere : public geometry {
  public:
    sphere(const vec3& center, double radius, shared_ptr<material> mat)
      : center_(center), radius_(std::fabs(radius)), mat_(std::move(mat)) {}

    const vec3& center() const { return center_; }
    double radius() const { return radius_; }

    bool hit(const ray& r, const interval* t_range, intersection& isect) const override;
    vec3 normal(const vec3& point) const override;

  private:
    vec3 center_;
    double radius_;
    shared_ptr<material> mat_;
};

bool sphere::hit(const ray& r, const interval* t_range, intersection& isect) const {
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
  isect.mat = mat_;
  return true;
}

vec3 sphere::normal(const vec3& point) const {
  return (point - center_);
}

#endif
