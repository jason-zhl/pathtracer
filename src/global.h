#ifndef GLOBAL_H
#define GLOBAL_H

#include <limits>
#include <memory>

using std::make_shared;
using std::shared_ptr;
using std::make_unique;
using std::unique_ptr;

constexpr float INF = std::numeric_limits<float>::infinity();
constexpr float PI = 3.14159265358979323846f;
constexpr float INV_PI = 1.0f / PI;
constexpr float INV_2PI = 1.0f / (2.0f * PI);

#include "rng.h"

#include "interval.h"
#include "vec3.h"
#include "ray.h"
#include "color.h"
#endif
