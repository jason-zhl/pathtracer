#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "vec3.h"

class ray;
class geometry;

class intersection {
  public:
    vec3 point;
    double t = 0;
    const geometry* surface = nullptr;
};

class geometry {
  public:
    virtual ~geometry() = default;
    virtual bool hit(const ray& r, intersection& isect) const = 0;
    virtual vec3 normal(const vec3& point) const = 0;
};

#endif
