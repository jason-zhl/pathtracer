#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class ray {
  public:
    ray() {}
    ray(const vec3& origin, const vec3& direction) : origin_(origin), direction_(direction) {}

    const vec3& origin() const { return origin_; }
    const vec3& direction() const { return direction_; }

    vec3 at(double t) const {
      return origin_ + t * direction_;
    }

  private:
    vec3 origin_;
    vec3 direction_;
};

#endif