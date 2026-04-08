#ifndef SPHERE_H
#define SPHERE_H

#include "vec3.h"

class sphere {
  public:
    sphere(const vec3& center, double radius) : center_(center), radius_(radius) {}

    const vec3& center() const { return center_; }
    double radius() const { return radius_; }

    double hit(const ray& r) const;
  private:
    vec3 center_;
    double radius_;
};

double sphere::hit(const ray& r) const {
  vec3 oc = r.origin() - center_;
  auto a = dot(r.direction(), r.direction());
  auto b = 2.0 * dot(oc, r.direction());
  auto c = dot(oc, oc) - radius_ * radius_;
  auto discriminant = b * b - 4 * a * c;
  if (discriminant < 0) {
    return -1.0;
  }
  return (-b + std::sqrt(discriminant)) / (2.0 * a);
}

#endif