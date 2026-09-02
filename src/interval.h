#ifndef INTERVAL_H
#define INTERVAL_H

#include "host_device.h"

#include <cmath>

class interval {
  public:
    double min;
    double max;

    HOST_DEVICE interval() : min(INFINITY), max(-INFINITY) {}

    HOST_DEVICE interval(double mn, double mx) : min(mn), max(mx) {}

    /** Accepted ray parameter range [min, max) (half-open on the right). */
    HOST_DEVICE bool surrounds(double t) const {
      return t >= min && t < max;
    }

    HOST_DEVICE bool contains_closed(double t) const {
      return min <= t && t <= max;
    }
};

#endif
