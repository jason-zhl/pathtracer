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
};

#endif
