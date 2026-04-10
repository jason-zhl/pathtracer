#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "global.h"

class material;
class intersection;

class geometry {
  public:
    virtual ~geometry() = default;
    virtual bool hit(const ray& r, const interval* t_range, intersection& isect) const = 0;
    virtual vec3 normal(const vec3& point) const = 0;

    /** Uniform area sample for NEE; pdf w.r.t. surface area. */
    virtual bool sample_emitter_point(vec3& p, vec3& n, double& pdf_area) const {
      (void)p;
      (void)n;
      (void)pdf_area;
      return false;
    }

    /** Finite surface measure for emitters (0 if not used as area light). */
    virtual double surface_area() const { return 0.0; }
};

class intersection {
  public:
    vec3 point;
    double t = 0;
    const geometry* surface = nullptr;
    shared_ptr<material> mat;
};

#endif
