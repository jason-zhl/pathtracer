#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"
#include "geometry.h"
#include "ray.h"

class material {
  public:
    virtual ~material() = default;

    virtual bool scatter(const ray& r_in, const intersection& rec, color& attenuation,
      ray& scattered) const = 0;

    /** BSDF f(wi, wo); wi is toward incoming ray. For MIS with environment sampling. */
    virtual color eval(const ray& r_in, const intersection& rec, const vec3& wo) const {
      return color(0, 0, 0);
    }

    /** PDF for wo with respect to solid angle (hemisphere / mixture as in scatter). */
    virtual double pdf(const ray& r_in, const intersection& rec, const vec3& wo) const { return 0.0; }
};

#endif
