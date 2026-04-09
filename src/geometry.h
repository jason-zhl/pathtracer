#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "global.h"

class intersection;

class geometry {
  public:
    virtual ~geometry() = default;
    virtual bool hit(const ray& r, intersection& isect) const = 0;
    virtual vec3 normal(const vec3& point) const = 0;
};

class intersection {
  public:
    vec3 point;
    double t = 0;
    const geometry* surface = nullptr;
};

#endif
