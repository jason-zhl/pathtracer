#ifndef CUDA_RAY_COLOUR_H
#define CUDA_RAY_COLOUR_H

#include "color.h"

#include <cstdint>

class camera;
struct Scene;

bool cuda_ray_colour(color* pixels, int64_t total_pixels, const camera& cam, const Scene& scene);

#endif
