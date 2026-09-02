#ifndef IBL_H
#define IBL_H

vec3 ibl_value(const Environment& env, const vec3& direction);
void ibl_sample_direction(const Environment& env, vec3& out_direction,
  double& out_pdf_solid_angle, RNG& rng);
double ibl_pdf(const Environment& env, const vec3& direction);

#endif
