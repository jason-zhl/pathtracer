#ifndef SOLID_H
#define SOLID_H

#include "environment.h"

/** Constant radiance environment (no importance sampling; pdf is always 0). */
class solid : public environment {
  public:
    explicit solid(const vec3& colour) : colour_(colour) {}

    vec3 value(const vec3& /*direction*/) const override { return colour_; }

    void sample_direction(vec3& out_direction, double& out_pdf_solid_angle) const override {
      out_pdf_solid_angle = 0.0;
      out_direction = vec3(0, 1, 0);
    }

    double pdf(const vec3& /*direction*/) const override { return 0.0; }

  private:
    vec3 colour_;
};

#endif
