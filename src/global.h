#ifndef GLOBAL_H
#define GLOBAL_H

#include <cmath>
#include <limits>
#include <memory>
#include <random>

using std::make_shared;
using std::shared_ptr;
using std::make_unique;
using std::unique_ptr;

const double INF = std::numeric_limits<double>::infinity();
const double PI = std::acos(-1.0);
const double INV_PI = 1.0 / PI;
const double INV_2PI = 1.0 / (2.0 * PI);

inline std::mt19937& random_generator() {
  static std::mt19937 gen{std::random_device{}()};
  return gen;
}

inline double random_double() {
  static std::uniform_real_distribution<double> dist(0.0, 1.0);
  return dist(random_generator());
}

inline double random_double(double min, double max) {
  return min + (max - min) * random_double();
}

#include "interval.h"
#include "vec3.h"
#include "ray.h"
#include "color.h"
#endif