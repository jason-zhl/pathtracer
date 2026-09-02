#ifndef LAMBERTIAN_H
#define LAMBERTIAN_H

inline bool lambertian_scatter(const Material& mat, const ray& r_in, const intersection& rec,
  color& attenuation, ray& scattered) {
  vec3 n = unit_vector(rec.surface->normal(rec.point));
  if (dot(r_in.direction(), n) > 0.0) {
    n = -n;
  }
  const vec3 scatter_direction = unit_vector(lambertian_random(n));
  const double surface_offset = 1e-3;
  scattered = ray(rec.point + n * surface_offset, scatter_direction);
  attenuation = mat.albedo;
  return true;
}

inline color lambertian_eval(const Material& mat, const ray& r_in, const intersection& rec,
  const vec3& wo) {
  vec3 n = unit_vector(rec.surface->normal(rec.point));
  if (dot(r_in.direction(), n) > 0.0) {
    n = -n;
  }
  const double ndotwo = dot(n, unit_vector(wo));
  if (ndotwo <= 0.0) {
    return color(0, 0, 0);
  }
  return mat.albedo / PI;
}

inline double lambertian_pdf(const Material& mat, const ray& r_in, const intersection& rec,
  const vec3& wo) {
  (void)mat;
  vec3 n = unit_vector(rec.surface->normal(rec.point));
  if (dot(r_in.direction(), n) > 0.0) {
    n = -n;
  }
  const double ndotwo = dot(n, unit_vector(wo));
  if (ndotwo <= 0.0) {
    return 0.0;
  }
  return ndotwo / PI;
}

#endif
