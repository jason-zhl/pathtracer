#ifndef PLANE_PATCH_H
#define PLANE_PATCH_H

HOST_DEVICE inline bool plane_hit(const Geometry& g, const ray& r, const interval* t_range,
  intersection& isect) {
  if (t_range == nullptr) {
    return false;
  }
  if (g.n_len_sq < 1e-30f) {
    return false;
  }
  float denom = dot(r.direction(), g.n);
  if (fabsf(denom) < 1e-12f) {
    return false;
  }
  float t = dot(g.corner - r.origin(), g.n) / denom;
  if (!t_range->surrounds(t)) {
    return false;
  }
  vec3 p = r.at(t);
  vec3 w = p - g.corner;
  float s = dot(cross(w, g.v), g.n) / g.n_len_sq;
  float tv = dot(cross(g.u, w), g.n) / g.n_len_sq;
  if (s < 0.0f || s > 1.0f || tv < 0.0f || tv > 1.0f) {
    return false;
  }
  isect.point = p;
  isect.t = t;
  isect.normal = g.n;
  isect.mat_id = g.mat_id;
  return true;
}

HOST_DEVICE inline vec3 plane_normal(const Geometry& g, const vec3& point) {
  (void)point;
  return g.n;
}

HOST_DEVICE inline bool plane_sample_emitter_point(const Geometry& g, vec3& p, vec3& n,
  float& pdf_area, RNG& rng) {
  const float su = rng.next();
  const float sv = rng.next();
  p = g.corner + su * g.u + sv * g.v;
  n = unit_vector(g.n);
  const float a = sqrtf(g.n_len_sq);
  if (a < 1e-30f) {
    return false;
  }
  pdf_area = 1.0f / a;
  return true;
}

HOST_DEVICE inline float plane_surface_area(const Geometry& g) {
  return sqrtf(g.n_len_sq);
}

#endif
