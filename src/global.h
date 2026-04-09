#ifndef GLOBAL_H
#define GLOBAL_H

#include <random>

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

#endif
