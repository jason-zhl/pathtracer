#ifndef INTERVAL_H
#define INTERVAL_H

#include <limits>

class interval {
  public:
    double min;
    double max;

    interval()
      : min(std::numeric_limits<double>::infinity()),
        max(-std::numeric_limits<double>::infinity()) {}

    interval(double mn, double mx) : min(mn), max(mx) {}

    /** Accepted ray parameter range [min, max) (half-open on the right). */
    bool surrounds(double t) const {
      return t >= min && t < max;
    }

    bool contains_closed(double t) const {
      return min <= t && t <= max;
    }
};

#endif
