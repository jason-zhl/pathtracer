#include "global.h"
#include "camera.h"
#include "cuda/ray_colour.h"
#include "trace.h"

#include <cuda_runtime.h>

#include <iostream>

namespace {

__global__ void ray_colour_kernel(float* rgb, camera cam, Scene scene) {
  const int x = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
  const int y = static_cast<int>(blockIdx.y * blockDim.y + threadIdx.y);
  const int width = cam.image_width();
  const int height = cam.image_height();
  if (x >= width || y >= height) {
    return;
  }

  color pixel(0, 0, 0);
  const int spp = cam.samples_per_pixel();
  for (int s = 0; s < spp; ++s) {
    RNG rng = make_rng(x, y, s);
    pixel += ray_colour(cam.primary_ray(x, y, rng), cam, scene, rng);
  }
  pixel /= static_cast<float>(spp);

  const int idx = (y * width + x) * 3;
  rgb[idx + 0] = pixel.x();
  rgb[idx + 1] = pixel.y();
  rgb[idx + 2] = pixel.z();
}

struct DeviceScene {
  Geometry* geometries = nullptr;
  Material* materials = nullptr;
  int* area_lights = nullptr;
  float* rgb = nullptr;

  void free_all() {
    cudaFree(geometries);
    cudaFree(materials);
    cudaFree(area_lights);
    cudaFree(rgb);
    geometries = nullptr;
    materials = nullptr;
    area_lights = nullptr;
    rgb = nullptr;
  }
};

}  // namespace

bool cuda_ray_colour(color* pixels, int64_t total_pixels, const camera& cam, const Scene& scene) {
  if (pixels == nullptr || total_pixels <= 0) {
    return false;
  }

  const int width = cam.image_width();
  const int height = cam.image_height();
  if (width <= 0 || height <= 0 || static_cast<int64_t>(width) * height != total_pixels) {
    std::cerr << "cuda_ray_colour: width * height != total_pixels\n";
    return false;
  }

  static_assert(sizeof(color) == 3 * sizeof(float),
                "color must be three floats for CUDA memcpy");

  DeviceScene device;
  Scene gpu_scene = scene;
  gpu_scene.env = nullptr;

#define CUDA_CHECK(call)                                                       \
  do {                                                                         \
    const cudaError_t err = (call);                                            \
    if (err != cudaSuccess) {                                                   \
      std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__ << ": "     \
                << cudaGetErrorString(err) << '\n';                          \
      device.free_all();                                                       \
      return false;                                                           \
    }                                                                          \
  } while (0)

  CUDA_CHECK(cudaDeviceSetLimit(cudaLimitStackSize, 64 * 1024));

  if (scene.n_geometries > 0) {
    CUDA_CHECK(cudaMalloc(&device.geometries,
      static_cast<std::size_t>(scene.n_geometries) * sizeof(Geometry)));
    CUDA_CHECK(cudaMemcpy(device.geometries, scene.geometries,
      static_cast<std::size_t>(scene.n_geometries) * sizeof(Geometry),
      cudaMemcpyHostToDevice));
  }
  if (scene.n_materials > 0) {
    CUDA_CHECK(cudaMalloc(&device.materials,
      static_cast<std::size_t>(scene.n_materials) * sizeof(Material)));
    CUDA_CHECK(cudaMemcpy(device.materials, scene.materials,
      static_cast<std::size_t>(scene.n_materials) * sizeof(Material),
      cudaMemcpyHostToDevice));
  }
  if (scene.n_area_lights > 0) {
    CUDA_CHECK(cudaMalloc(&device.area_lights,
      static_cast<std::size_t>(scene.n_area_lights) * sizeof(int)));
    CUDA_CHECK(cudaMemcpy(device.area_lights, scene.area_lights,
      static_cast<std::size_t>(scene.n_area_lights) * sizeof(int),
      cudaMemcpyHostToDevice));
  }

  gpu_scene.geometries = device.geometries;
  gpu_scene.materials = device.materials;
  gpu_scene.area_lights = device.area_lights;

  const std::size_t byte_count =
      static_cast<std::size_t>(total_pixels) * 3 * sizeof(float);
  CUDA_CHECK(cudaMalloc(&device.rgb, byte_count));

  const dim3 block(16, 16);
  const dim3 grid((width + block.x - 1) / block.x,
                  (height + block.y - 1) / block.y);
  ray_colour_kernel<<<grid, block>>>(device.rgb, cam, gpu_scene);
  CUDA_CHECK(cudaGetLastError());
  CUDA_CHECK(cudaDeviceSynchronize());

  CUDA_CHECK(cudaMemcpy(pixels, device.rgb, byte_count, cudaMemcpyDeviceToHost));
  device.free_all();

#undef CUDA_CHECK
  return true;
}
