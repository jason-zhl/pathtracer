#ifndef HOST_DEVICE_H
#define HOST_DEVICE_H

#include <cmath>

#ifdef __CUDACC__
#define HOST_DEVICE __host__ __device__
#else
#define HOST_DEVICE
#endif

HOST_DEVICE inline double hd_sqrt(double x) {
#ifdef __CUDA_ARCH__
  return sqrt(x);
#else
  return std::sqrt(x);
#endif
}

HOST_DEVICE inline double hd_copysign(double mag, double sgn) {
#ifdef __CUDA_ARCH__
  return copysign(mag, sgn);
#else
  return std::copysign(mag, sgn);
#endif
}

HOST_DEVICE inline double hd_sin(double x) {
#ifdef __CUDA_ARCH__
  return sin(x);
#else
  return std::sin(x);
#endif
}

HOST_DEVICE inline double hd_cos(double x) {
#ifdef __CUDA_ARCH__
  return cos(x);
#else
  return std::cos(x);
#endif
}

#endif
