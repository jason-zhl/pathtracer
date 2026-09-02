#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class ray {
  public:
    HOST_DEVICE ray() {}
    HOST_DEVICE ray(const vec3& origin, const vec3& direction)
      : origin_(origin), direction_(direction) {}

    HOST_DEVICE const vec3& origin() const { return origin_; }
    HOST_DEVICE const vec3& direction() const { return direction_; }

    HOST_DEVICE vec3 at(double t) const {
      return origin_ + t * direction_;
    }

  private:
    vec3 origin_;
    vec3 direction_;
};

#endif
