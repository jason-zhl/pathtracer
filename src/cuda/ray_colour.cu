#include "global.h"
#include "cuda/ray_colour.h"

#include <cuda_runtime.h>

#include <iostream>

namespace {

__global__ void ray_colour_kernel(double* rgb, int width, int height) {
  const int x = blockIdx.x * blockDim.x + threadIdx.x;
  const int y = blockIdx.y * blockDim.y + threadIdx.y;
  if (x >= width || y >= height) {
    return;
  }

  const int idx = (y * width + x) * 3;
  // Placeholder until the path-tracing ray_colour port lives here.
  const color pixel(static_cast<double>(x) / width,
                   static_cast<double>(y) / height,
                   0.5);
  rgb[idx + 0] = pixel.x();
  rgb[idx + 1] = pixel.y();
  rgb[idx + 2] = pixel.z();
}

}  // namespace

bool cuda_ray_colour(color* pixels, int64_t total_pixels, int width, int height) {
  if (pixels == nullptr || total_pixels <= 0 || width <= 0 || height <= 0) {
    return false;
  }
  if (static_cast<int64_t>(width) * height != total_pixels) {
    std::cerr << "cuda_ray_colour: width * height != total_pixels\n";
    return false;
  }

  static_assert(sizeof(color) == 3 * sizeof(double),
                "color must be three doubles for CUDA memcpy");

  double* device_rgb = nullptr;

#define CUDA_CHECK(call)                                                       \
  do {                                                                         \
    const cudaError_t err = (call);                                            \
    if (err != cudaSuccess) {                                                  \
      std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__ << ": "     \
                << cudaGetErrorString(err) << '\n';                           \
      if (device_rgb != nullptr) {                                             \
        cudaFree(device_rgb);                                                  \
      }                                                                        \
      return false;                                                            \
    }                                                                          \
  } while (0)

  const std::size_t byte_count =
      static_cast<std::size_t>(total_pixels) * 3 * sizeof(double);

  CUDA_CHECK(cudaMalloc(&device_rgb, byte_count));

  const dim3 block(16, 16);
  const dim3 grid((width + block.x - 1) / block.x,
                  (height + block.y - 1) / block.y);
  ray_colour_kernel<<<grid, block>>>(device_rgb, width, height);
  CUDA_CHECK(cudaGetLastError());
  CUDA_CHECK(cudaDeviceSynchronize());

  CUDA_CHECK(cudaMemcpy(pixels, device_rgb, byte_count, cudaMemcpyDeviceToHost));
  CUDA_CHECK(cudaFree(device_rgb));
  device_rgb = nullptr;

#undef CUDA_CHECK
  return true;
}
