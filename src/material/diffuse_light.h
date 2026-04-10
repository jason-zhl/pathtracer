#ifndef DIFFUSE_LIGHT_H
#define DIFFUSE_LIGHT_H

#include "material.h"

// One-sided diffuse emitter: emits on the face the ray hits first (geometric normal · wi < 0).
class diffuse_light : public material {
  public:
    explicit diffuse_light(const color& emit) : emit_(emit) {}

    bool scatter(const ray& r_in, const intersection& rec, color& attenuation,
      ray& scattered) const override {
      (void)r_in;
      (void)rec;
      (void)attenuation;
      (void)scattered;
      return false;
    }

    color emitted(const ray& r_in, const intersection& rec) const {
      const vec3 gn = unit_vector(rec.surface->normal(rec.point));
      if (dot(r_in.direction(), gn) < 0.0) {
        return emit_;
      }
      return color(0, 0, 0);
    }

  private:
    color emit_;
};

#endif
