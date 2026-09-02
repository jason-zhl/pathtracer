#ifndef SOLID_H
#define SOLID_H

inline vec3 solid_value(const Environment& env, const vec3& /*direction*/) {
  return env.colour;
}

inline void solid_sample_direction(const Environment& env, vec3& out_direction,
  double& out_pdf_solid_angle, RNG& rng) {
  (void)env;
  (void)rng;
  out_pdf_solid_angle = 0.0;
  out_direction = vec3(0, 1, 0);
}

inline double solid_pdf(const Environment& env, const vec3& /*direction*/) {
  (void)env;
  return 0.0;
}

#endif
