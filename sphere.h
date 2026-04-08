#ifndef SPHERE_H
#define SPHERE_H

#include "ray.h"
#include "vec3.h"
#include "geometry.h"

class sphere : public geometry {
  public:
    sphere(const vec3& center, double radius) : center_(center), radius_(std::fabs(radius)) {}

    const vec3& center() const { return center_; }
    double radius() const { return radius_; }

    bool hit(const ray& r, intersection& isect) const override;
    vec3 normal(const vec3& point) const override;

  private:
    vec3 center_;
    double radius_;
};

bool sphere::hit(const ray& r, intersection& isect) const {
  vec3 oc = r.origin() - center_;
  auto a = dot(r.direction(), r.direction());
  auto b = 2.0 * dot(oc, r.direction());
  auto c = dot(oc, oc) - radius_ * radius_;
  auto discriminant = b * b - 4 * a * c;
  if (discriminant < 0) {
    return false;
  }
  auto t = (-b - std::sqrt(discriminant)) / (2.0 * a);
  if (t < 0) {
    return false;
  }
  isect.point = r.at(t);
  isect.t = t;
  isect.surface = this;
  return true;
}

vec3 sphere::normal(const vec3& point) const {
  return (point - center_);
}

#endif
