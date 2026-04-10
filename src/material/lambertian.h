#ifndef LAMBERTIAN_H
#define LAMBERTIAN_H

#include "material.h"

class lambertian : public material {
  public:
    explicit lambertian(const color& albedo) : albedo_(albedo) {}

    bool scatter(const ray& r_in, const intersection& rec, color& attenuation,
      ray& scattered) const override {
      vec3 n = unit_vector(rec.surface->normal(rec.point));
      if (dot(r_in.direction(), n) > 0.0) {
        n = -n;
      }
      const vec3 scatter_direction = unit_vector(lambertian_random(n));
      const double surface_offset = 1e-3;
      scattered = ray(rec.point + n * surface_offset, scatter_direction);
      attenuation = albedo_;
      return true;
    }

    color eval(const ray& r_in, const intersection& rec, const vec3& wo) const override {
      vec3 n = unit_vector(rec.surface->normal(rec.point));
      if (dot(r_in.direction(), n) > 0.0) {
        n = -n;
      }
      const double ndotwo = dot(n, unit_vector(wo));
      if (ndotwo <= 0.0) {
        return color(0, 0, 0);
      }
      return albedo_ / PI;
    }

    double pdf(const ray& r_in, const intersection& rec, const vec3& wo) const override {
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

  private:
    color albedo_;
};

#endif
