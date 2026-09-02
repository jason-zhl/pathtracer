#ifndef CUDA_RAY_COLOUR_H
#define CUDA_RAY_COLOUR_H

#include "color.h"

#include <cstdint>

// Fills pixels[0..total_pixels) on the GPU. Returns false if a CUDA error occurred.
// width * height must equal total_pixels.
bool cuda_ray_colour(color* pixels, int64_t total_pixels, int width, int height);

#endif
