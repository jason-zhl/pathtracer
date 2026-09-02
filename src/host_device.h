#ifndef HOST_DEVICE_H
#define HOST_DEVICE_H

#include <math.h>

#ifdef __CUDACC__
#define HOST_DEVICE __host__ __device__
#else
#define HOST_DEVICE
#endif

HOST_DEVICE inline double clamp(double x, double lo, double hi) {
  return fmin(hi, fmax(lo, x));
}

HOST_DEVICE inline int clamp(int x, int lo, int hi) {
  return x < lo ? lo : (x > hi ? hi : x);
}

#endif
