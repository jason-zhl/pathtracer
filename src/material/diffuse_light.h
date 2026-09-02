#ifndef DIFFUSE_LIGHT_H
#define DIFFUSE_LIGHT_H

HOST_DEVICE inline bool diffuse_light_scatter(const Material& mat, const ray& r_in, const intersection& rec,
  color& attenuation, ray& scattered, RNG& rng) {
  (void)mat;
  (void)r_in;
  (void)rec;
  (void)attenuation;
  (void)scattered;
  (void)rng;
  return false;
}

HOST_DEVICE inline color diffuse_light_emitted(const Material& mat, const ray& r_in, const intersection& rec) {
  const vec3 gn = unit_vector(rec.normal);
  if (dot(r_in.direction(), gn) < 0.0f) {
    return mat.emission;
  }
  return color(0, 0, 0);
}

#endif
