#ifndef LAMBERTIAN_H
#define LAMBERTIAN_H

HOST_DEVICE inline bool lambertian_scatter(const Material& mat, const ray& r_in, const intersection& rec,
  color& attenuation, ray& scattered, RNG& rng) {
  vec3 n = unit_vector(rec.normal);
  if (dot(r_in.direction(), n) > 0.0f) {
    n = -n;
  }
  const vec3 scatter_direction = unit_vector(lambertian_random(n, rng));
  const float surface_offset = 1e-3f;
  scattered = ray(rec.point + n * surface_offset, scatter_direction);
  attenuation = mat.albedo;
  return true;
}

HOST_DEVICE inline color lambertian_eval(const Material& mat, const ray& r_in, const intersection& rec,
  const vec3& wo) {
  vec3 n = unit_vector(rec.normal);
  if (dot(r_in.direction(), n) > 0.0f) {
    n = -n;
  }
  const float ndotwo = dot(n, unit_vector(wo));
  if (ndotwo <= 0.0f) {
    return color(0, 0, 0);
  }
  return mat.albedo / PI;
}

HOST_DEVICE inline float lambertian_pdf(const Material& mat, const ray& r_in, const intersection& rec,
  const vec3& wo) {
  (void)mat;
  vec3 n = unit_vector(rec.normal);
  if (dot(r_in.direction(), n) > 0.0f) {
    n = -n;
  }
  const float ndotwo = dot(n, unit_vector(wo));
  if (ndotwo <= 0.0f) {
    return 0.0f;
  }
  return ndotwo / PI;
}

#endif
