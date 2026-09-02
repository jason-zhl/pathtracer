#ifndef SOLID_H
#define SOLID_H

HOST_DEVICE inline vec3 solid_value(const Environment& env, const vec3& /*direction*/) {
  return env.colour;
}

HOST_DEVICE inline void solid_sample_direction(const Environment& env, vec3& out_direction,
  float& out_pdf_solid_angle, RNG& rng) {
  (void)env;
  (void)rng;
  out_pdf_solid_angle = 0.0f;
  out_direction = vec3(0, 1, 0);
}

HOST_DEVICE inline float solid_pdf(const Environment& env, const vec3& /*direction*/) {
  (void)env;
  return 0.0f;
}

#endif
