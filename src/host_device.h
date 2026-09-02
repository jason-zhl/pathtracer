#ifndef HOST_DEVICE_H
#define HOST_DEVICE_H

#include <math.h>

#ifdef __CUDACC__
#define HOST_DEVICE __host__ __device__
#else
#define HOST_DEVICE
#endif

HOST_DEVICE inline float clamp(float x, float lo, float hi) {
  return fminf(hi, fmaxf(lo, x));
}

HOST_DEVICE inline int clamp(int x, int lo, int hi) {
  return x < lo ? lo : (x > hi ? hi : x);
}

#endif
