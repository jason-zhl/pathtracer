#ifndef SPHERE_H
#define SPHERE_H

HOST_DEVICE inline bool sphere_hit(const Geometry& g, const ray& r, const interval* t_range,
  intersection& isect) {
  if (t_range == nullptr) {
    return false;
  }
  vec3 oc = r.origin() - g.center;
  auto a = dot(r.direction(), r.direction());
  auto b = 2.0f * dot(oc, r.direction());
  auto c = dot(oc, oc) - g.radius * g.radius;
  auto discriminant = b * b - 4 * a * c;
  if (discriminant < 0) {
    return false;
  }
  const auto sqrt_d = sqrtf(discriminant);
  auto t = (-b - sqrt_d) / (2.0f * a);
  if (!t_range->surrounds(t)) {
    t = (-b + sqrt_d) / (2.0f * a);
    if (!t_range->surrounds(t)) {
      return false;
    }
  }
  isect.point = r.at(t);
  isect.t = t;
  isect.normal = isect.point - g.center;
  isect.mat_id = g.mat_id;
  return true;
}

HOST_DEVICE inline vec3 sphere_normal(const Geometry& g, const vec3& point) {
  return point - g.center;
}

HOST_DEVICE inline bool sphere_sample_emitter_point(const Geometry& g, vec3& p, vec3& n,
  float& pdf_area, RNG& rng) {
  n = random_unit_vector(rng);
  p = g.center + g.radius * n;
  pdf_area = 1.0f / (4.0f * PI * g.radius * g.radius);
  return true;
}

HOST_DEVICE inline float sphere_surface_area(const Geometry& g) {
  return 4.0f * PI * g.radius * g.radius;
}

#endif
