#ifndef DIFFUSE_LIGHT_H
#define DIFFUSE_LIGHT_H

inline bool diffuse_light_scatter(const Material& mat, const ray& r_in, const intersection& rec,
  color& attenuation, ray& scattered) {
  (void)mat;
  (void)r_in;
  (void)rec;
  (void)attenuation;
  (void)scattered;
  return false;
}

inline color diffuse_light_emitted(const Material& mat, const ray& r_in, const intersection& rec) {
  const vec3 gn = unit_vector(rec.normal);
  if (dot(r_in.direction(), gn) < 0.0) {
    return mat.emission;
  }
  return color(0, 0, 0);
}

#endif
