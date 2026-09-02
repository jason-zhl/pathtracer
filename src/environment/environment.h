#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "global.h"

#include <string>
#include <vector>

enum class EnvType {
  Solid,
  IBL
};

struct Environment {
  EnvType type = EnvType::Solid;
  vec3 colour;

  std::vector<float> texture;
  int width = 0;
  int height = 0;
  std::vector<double> marginal_cdf;
  std::vector<double> cond_cdf;
  std::vector<double> pixel_weights;
  double total_weight = 0.0;

  static Environment solid(const vec3& colour);
  static Environment ibl(const std::string& file_name);

  HOST_DEVICE vec3 value(const vec3& direction) const;
  HOST_DEVICE void sample_direction(vec3& out_direction, double& out_pdf_solid_angle, RNG& rng) const;
  HOST_DEVICE double pdf(const vec3& direction) const;
};

inline Environment Environment::solid(const vec3& colour) {
  Environment env;
  env.type = EnvType::Solid;
  env.colour = colour;
  return env;
}

#include "solid.h"
#include "ibl.h"

HOST_DEVICE inline vec3 Environment::value(const vec3& direction) const {
  switch (type) {
    case EnvType::Solid:
      return solid_value(*this, direction);
    case EnvType::IBL:
      return ibl_value(*this, direction);
  }
  return vec3();
}

HOST_DEVICE inline void Environment::sample_direction(vec3& out_direction, double& out_pdf_solid_angle,
  RNG& rng) const {
  switch (type) {
    case EnvType::Solid:
      solid_sample_direction(*this, out_direction, out_pdf_solid_angle, rng);
      return;
    case EnvType::IBL:
      ibl_sample_direction(*this, out_direction, out_pdf_solid_angle, rng);
      return;
  }
  out_pdf_solid_angle = 0.0;
  out_direction = vec3(0, 1, 0);
}

HOST_DEVICE inline double Environment::pdf(const vec3& direction) const {
  switch (type) {
    case EnvType::Solid:
      return solid_pdf(*this, direction);
    case EnvType::IBL:
      return ibl_pdf(*this, direction);
  }
  return 0.0;
}

#endif
