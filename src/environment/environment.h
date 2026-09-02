#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "../global.h"

/** Infinite environment / background: radiance lookup and optional importance sampling. */
class environment {
  public:
    virtual ~environment() = default;

    virtual vec3 value(const vec3& direction) const = 0;

    /** Sample a direction; pdf w.r.t. solid angle. May set pdf to 0 if sampling is unsupported. */
    virtual void sample_direction(vec3& out_direction, double& out_pdf_solid_angle,
      RNG& rng) const = 0;

    /** Pdf for env sampling at `direction` (consistent with sample_direction). */
    virtual double pdf(const vec3& direction) const = 0;
};

#endif
