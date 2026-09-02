#ifndef GLOBAL_H
#define GLOBAL_H

#include <limits>
#include <memory>

using std::make_shared;
using std::shared_ptr;
using std::make_unique;
using std::unique_ptr;

constexpr double INF = std::numeric_limits<double>::infinity();
constexpr double PI = 3.14159265358979323846;
constexpr double INV_PI = 1.0 / PI;
constexpr double INV_2PI = 1.0 / (2.0 * PI);

#include "rng.h"

#include "interval.h"
#include "vec3.h"
#include "ray.h"
#include "color.h"
#endif
