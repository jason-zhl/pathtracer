#include "environment/environment.h"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {

inline int texel_i(double u, int width) {
  const double uu = u - std::floor(u);
  const int i = static_cast<int>(uu * static_cast<double>(width));
  return std::min(std::max(i, 0), width - 1);
}

inline int texel_j(double v, int height) {
  const double vv = std::clamp(v, 0.0, 1.0);
  const double fy = (1.0 - vv) * static_cast<double>(height - 1);
  const int j = static_cast<int>(fy);
  return std::min(std::max(j, 0), height - 1);
}

inline double luminance_rgb(double r, double g, double b) {
  return 0.212671 * r + 0.71516 * g + 0.072169 * b;
}

void uv_from_direction(const vec3& d, double& u, double& v) {
  const vec3 du = unit_vector(d);
  const double phi = std::atan2(du.z(), du.x());
  const double theta = std::asin(std::clamp(du.y(), -1.0, 1.0));
  u = phi * INV_2PI;
  u = u - std::floor(u);
  v = theta * INV_PI + 0.5;
}

void direction_from_uv(double u, double v, vec3& out) {
  double uu = std::fmod(u, 1.0);
  if (uu < 0.0) {
    uu += 1.0;
  }
  const double vv = std::clamp(v, 0.0, 1.0);
  const double phi = uu * (2.0 * PI);
  const double theta = (vv - 0.5) * PI;
  const double cos_t = std::cos(theta);
  out = vec3(cos_t * std::cos(phi), std::sin(theta), cos_t * std::sin(phi));
}

vec3 bilinear_interpolation(const Environment& env, double u, double v) {
  const double fx = u * static_cast<double>(env.width - 1);
  const double fy = (1.0 - v) * static_cast<double>(env.height - 1);

  const int i0 = static_cast<int>(fx);
  const int j0 = static_cast<int>(fy);
  const int i1 = std::min(i0 + 1, env.width - 1);
  const int j1 = std::min(j0 + 1, env.height - 1);

  const double tx = fx - static_cast<double>(i0);
  const double ty = fy - static_cast<double>(j0);

  auto texel = [&env](int i, int j) -> vec3 {
    const int idx = (j * env.width + i) * 3;
    return vec3(static_cast<double>(env.texture[static_cast<size_t>(idx) + 0]),
                static_cast<double>(env.texture[static_cast<size_t>(idx) + 1]),
                static_cast<double>(env.texture[static_cast<size_t>(idx) + 2]));
  };

  const vec3 c00 = texel(i0, j0);
  const vec3 c10 = texel(i1, j0);
  const vec3 c01 = texel(i0, j1);
  const vec3 c11 = texel(i1, j1);

  const vec3 c0 = c00 * (1.0 - tx) + c10 * tx;
  const vec3 c1 = c01 * (1.0 - tx) + c11 * tx;
  return c0 * (1.0 - ty) + c1 * ty;
}

vec3 sample_equirectangular(const Environment& env, double u, double v) {
  double uu = std::fmod(u, 1.0);
  if (uu < 0.0) {
    uu += 1.0;
  }
  const double vv = std::clamp(v, 0.0, 1.0);
  return bilinear_interpolation(env, uu, vv);
}

void build_sampling_distribution(Environment& env) {
  env.pixel_weights.assign(static_cast<size_t>(env.width) * static_cast<size_t>(env.height), 0.0);
  env.marginal_cdf.assign(static_cast<size_t>(env.height) + 1, 0.0);
  env.cond_cdf.assign(static_cast<size_t>(env.height) * (static_cast<size_t>(env.width) + 1), 0.0);

  for (int j = 0; j < env.height; ++j) {
    const double v_center = 1.0 - (static_cast<double>(j) + 0.5) / static_cast<double>(env.height);
    const double theta = (v_center - 0.5) * PI;
    const double sin_theta = std::sin(theta);
    const double row_sin = std::max(sin_theta, 0.0);

    for (int i = 0; i < env.width; ++i) {
      const int idx = (j * env.width + i) * 3;
      const double r = static_cast<double>(env.texture[static_cast<size_t>(idx) + 0]);
      const double g = static_cast<double>(env.texture[static_cast<size_t>(idx) + 1]);
      const double b = static_cast<double>(env.texture[static_cast<size_t>(idx) + 2]);
      const double lum = luminance_rgb(r, g, b);
      env.pixel_weights[static_cast<size_t>(j * env.width + i)] = lum * row_sin + 1e-12;
    }

    const size_t row_off = static_cast<size_t>(j) * (static_cast<size_t>(env.width) + 1);
    env.cond_cdf[row_off] = 0.0;
    for (int i = 0; i < env.width; ++i) {
      env.cond_cdf[row_off + static_cast<size_t>(i) + 1] =
        env.cond_cdf[row_off + static_cast<size_t>(i)] +
        env.pixel_weights[static_cast<size_t>(j * env.width + i)];
    }

    const double row_sum = env.cond_cdf[row_off + static_cast<size_t>(env.width)];
    env.marginal_cdf[static_cast<size_t>(j) + 1] = env.marginal_cdf[static_cast<size_t>(j)] + row_sum;
  }

  env.total_weight = env.marginal_cdf[static_cast<size_t>(env.height)];
}

}  // namespace

Environment Environment::ibl(const std::string& file_name) {
  Environment env;
  env.type = EnvType::IBL;
  int channels = 0;
  float* data = stbi_loadf(file_name.c_str(), &env.width, &env.height, &channels, 3);
  if (!data) {
    throw std::runtime_error(std::string("Failed to load IBL texture: ") + stbi_failure_reason());
  }
  if (env.width < 1 || env.height < 1) {
    stbi_image_free(data);
    throw std::runtime_error("IBL texture has invalid dimensions");
  }
  const size_t n = static_cast<size_t>(env.width) * static_cast<size_t>(env.height) * 3;
  env.texture.assign(data, data + n);
  stbi_image_free(data);
  build_sampling_distribution(env);
  return env;
}

vec3 ibl_value(const Environment& env, const vec3& direction) {
  const vec3 d = unit_vector(direction);
  const double phi = std::atan2(d.z(), d.x());
  const double theta = std::asin(std::clamp(d.y(), -1.0, 1.0));

  const double u = phi * INV_2PI;
  const double v = theta * INV_PI + 0.5;

  return sample_equirectangular(env, u, v);
}

void ibl_sample_direction(const Environment& env, vec3& out_direction, double& out_pdf_solid_angle,
  RNG& rng) {
  if (env.total_weight <= 0.0) {
    out_pdf_solid_angle = 1.0 / (4.0 * PI);
    out_direction = random_unit_vector(rng);
    return;
  }

  const double r1 = rng.next() * env.total_weight;
  const auto row_it = std::upper_bound(env.marginal_cdf.begin(), env.marginal_cdf.end(), r1);
  int j = static_cast<int>(row_it - env.marginal_cdf.begin()) - 1;
  j = std::clamp(j, 0, env.height - 1);

  const double row_lo = env.marginal_cdf[static_cast<size_t>(j)];
  const double row_hi = env.marginal_cdf[static_cast<size_t>(j) + 1];
  const double row_sum = row_hi - row_lo;
  const double r2 = rng.next() * row_sum;

  const size_t row_off = static_cast<size_t>(j) * (static_cast<size_t>(env.width) + 1);
  const auto col_it = std::upper_bound(
    env.cond_cdf.begin() + static_cast<std::ptrdiff_t>(row_off),
    env.cond_cdf.begin() + static_cast<std::ptrdiff_t>(row_off + static_cast<size_t>(env.width) + 1),
    r2);
  int i = static_cast<int>(col_it - env.cond_cdf.begin() - static_cast<std::ptrdiff_t>(row_off)) - 1;
  i = std::clamp(i, 0, env.width - 1);

  const double u = (static_cast<double>(i) + 0.5) / static_cast<double>(env.width);
  const double v = 1.0 - (static_cast<double>(j) + 0.5) / static_cast<double>(env.height);
  direction_from_uv(u, v, out_direction);

  const double w_ij = env.pixel_weights[static_cast<size_t>(j * env.width + i)];
  out_pdf_solid_angle =
    w_ij * static_cast<double>(env.width * env.height) / (env.total_weight * 2.0 * PI * PI);
}

double ibl_pdf(const Environment& env, const vec3& direction) {
  if (env.total_weight <= 0.0) {
    return 1.0 / (4.0 * PI);
  }
  double u = 0.0;
  double v = 0.0;
  uv_from_direction(direction, u, v);
  const int i = texel_i(u, env.width);
  const int j = texel_j(v, env.height);
  const double w_ij = env.pixel_weights[static_cast<size_t>(j * env.width + i)];
  return w_ij * static_cast<double>(env.width * env.height) / (env.total_weight * 2.0 * PI * PI);
}
