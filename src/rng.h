#ifndef RNG_H
#define RNG_H

#include "host_device.h"

struct RNG {
  unsigned int state;

  HOST_DEVICE RNG() : state(1) {}
  HOST_DEVICE explicit RNG(unsigned int s) : state(s == 0 ? 1u : s) {}

  HOST_DEVICE double next() {
    state = state * 1664525u + 1013904223u;
    return static_cast<double>(state >> 8) * (1.0 / 16777216.0);
  }

  HOST_DEVICE double next(double lo, double hi) {
    return lo + (hi - lo) * next();
  }
};

HOST_DEVICE inline unsigned int wang_hash(unsigned int x) {
  x = (x ^ 61u) ^ (x >> 16);
  x *= 9u;
  x = x ^ (x >> 4);
  x *= 0x27d4eb2du;
  x = x ^ (x >> 15);
  return x;
}

HOST_DEVICE inline RNG make_rng(int x, int y, int sample, unsigned int seed = 0) {
  const unsigned int hashed = wang_hash(static_cast<unsigned int>(x) +
    wang_hash(static_cast<unsigned int>(y) +
      wang_hash(static_cast<unsigned int>(sample) + wang_hash(seed))));
  return RNG(hashed);
}

#endif
