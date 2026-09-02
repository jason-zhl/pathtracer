#include "environment/environment.h"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {

inline int texel_i(float u, int width) {
  const float uu = u - std::floor(u);
  const int i = static_cast<int>(uu * static_cast<float>(width));
  return std::min(std::max(i, 0), width - 1);
}

inline int texel_j(float v, int height) {
  const float vv = std::clamp(v, 0.0f, 1.0f);
  const float fy = (1.0f - vv) * static_cast<float>(height - 1);
  const int j = static_cast<int>(fy);
  return std::min(std::max(j, 0), height - 1);
}

inline float luminance_rgb(float r, float g, float b) {
  return 0.212671f * r + 0.71516f * g + 0.072169f * b;
}

void uv_from_direction(const vec3& d, float& u, float& v) {
  const vec3 du = unit_vector(d);
  const float phi = std::atan2(du.z(), du.x());
  const float theta = std::asin(std::clamp(du.y(), -1.0f, 1.0f));
  u = phi * INV_2PI;
  u = u - std::floor(u);
  v = theta * INV_PI + 0.5f;
}

void direction_from_uv(float u, float v, vec3& out) {
  float uu = std::fmod(u, 1.0f);
  if (uu < 0.0f) {
    uu += 1.0f;
  }
  const float vv = std::clamp(v, 0.0f, 1.0f);
  const float phi = uu * (2.0f * PI);
  const float theta = (vv - 0.5f) * PI;
  const float cos_t = std::cos(theta);
  out = vec3(cos_t * std::cos(phi), std::sin(theta), cos_t * std::sin(phi));
}

vec3 bilinear_interpolation(const Environment& env, float u, float v) {
  const float fx = u * static_cast<float>(env.width - 1);
  const float fy = (1.0f - v) * static_cast<float>(env.height - 1);

  const int i0 = static_cast<int>(fx);
  const int j0 = static_cast<int>(fy);
  const int i1 = std::min(i0 + 1, env.width - 1);
  const int j1 = std::min(j0 + 1, env.height - 1);

  const float tx = fx - static_cast<float>(i0);
  const float ty = fy - static_cast<float>(j0);

  auto texel = [&env](int i, int j) -> vec3 {
    const int idx = (j * env.width + i) * 3;
    return vec3(static_cast<float>(env.texture[static_cast<size_t>(idx) + 0]),
                static_cast<float>(env.texture[static_cast<size_t>(idx) + 1]),
                static_cast<float>(env.texture[static_cast<size_t>(idx) + 2]));
  };

  const vec3 c00 = texel(i0, j0);
  const vec3 c10 = texel(i1, j0);
  const vec3 c01 = texel(i0, j1);
  const vec3 c11 = texel(i1, j1);

  const vec3 c0 = c00 * (1.0f - tx) + c10 * tx;
  const vec3 c1 = c01 * (1.0f - tx) + c11 * tx;
  return c0 * (1.0f - ty) + c1 * ty;
}

vec3 sample_equirectangular(const Environment& env, float u, float v) {
  float uu = std::fmod(u, 1.0f);
  if (uu < 0.0f) {
    uu += 1.0f;
  }
  const float vv = std::clamp(v, 0.0f, 1.0f);
  return bilinear_interpolation(env, uu, vv);
}

void build_sampling_distribution(Environment& env) {
  env.pixel_weights.assign(static_cast<size_t>(env.width) * static_cast<size_t>(env.height), 0.0f);
  env.marginal_cdf.assign(static_cast<size_t>(env.height) + 1, 0.0f);
  env.cond_cdf.assign(static_cast<size_t>(env.height) * (static_cast<size_t>(env.width) + 1), 0.0f);

  for (int j = 0; j < env.height; ++j) {
    const float v_center = 1.0f - (static_cast<float>(j) + 0.5f) / static_cast<float>(env.height);
    const float theta = (v_center - 0.5f) * PI;
    const float sin_theta = std::sin(theta);
    const float row_sin = std::max(sin_theta, 0.0f);

    for (int i = 0; i < env.width; ++i) {
      const int idx = (j * env.width + i) * 3;
      const float r = static_cast<float>(env.texture[static_cast<size_t>(idx) + 0]);
      const float g = static_cast<float>(env.texture[static_cast<size_t>(idx) + 1]);
      const float b = static_cast<float>(env.texture[static_cast<size_t>(idx) + 2]);
      const float lum = luminance_rgb(r, g, b);
      env.pixel_weights[static_cast<size_t>(j * env.width + i)] = lum * row_sin + 1e-12f;
    }

    const size_t row_off = static_cast<size_t>(j) * (static_cast<size_t>(env.width) + 1);
    env.cond_cdf[row_off] = 0.0f;
    for (int i = 0; i < env.width; ++i) {
      env.cond_cdf[row_off + static_cast<size_t>(i) + 1] =
        env.cond_cdf[row_off + static_cast<size_t>(i)] +
        env.pixel_weights[static_cast<size_t>(j * env.width + i)];
    }

    const float row_sum = env.cond_cdf[row_off + static_cast<size_t>(env.width)];
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
  const float phi = std::atan2(d.z(), d.x());
  const float theta = std::asin(std::clamp(d.y(), -1.0f, 1.0f));

  const float u = phi * INV_2PI;
  const float v = theta * INV_PI + 0.5f;

  return sample_equirectangular(env, u, v);
}

void ibl_sample_direction(const Environment& env, vec3& out_direction, float& out_pdf_solid_angle,
  RNG& rng) {
  if (env.total_weight <= 0.0f) {
    out_pdf_solid_angle = 1.0f / (4.0f * PI);
    out_direction = random_unit_vector(rng);
    return;
  }

  const float r1 = rng.next() * env.total_weight;
  const auto row_it = std::upper_bound(env.marginal_cdf.begin(), env.marginal_cdf.end(), r1);
  int j = static_cast<int>(row_it - env.marginal_cdf.begin()) - 1;
  j = std::clamp(j, 0, env.height - 1);

  const float row_lo = env.marginal_cdf[static_cast<size_t>(j)];
  const float row_hi = env.marginal_cdf[static_cast<size_t>(j) + 1];
  const float row_sum = row_hi - row_lo;
  const float r2 = rng.next() * row_sum;

  const size_t row_off = static_cast<size_t>(j) * (static_cast<size_t>(env.width) + 1);
  const auto col_it = std::upper_bound(
    env.cond_cdf.begin() + static_cast<std::ptrdiff_t>(row_off),
    env.cond_cdf.begin() + static_cast<std::ptrdiff_t>(row_off + static_cast<size_t>(env.width) + 1),
    r2);
  int i = static_cast<int>(col_it - env.cond_cdf.begin() - static_cast<std::ptrdiff_t>(row_off)) - 1;
  i = std::clamp(i, 0, env.width - 1);

  const float u = (static_cast<float>(i) + 0.5f) / static_cast<float>(env.width);
  const float v = 1.0f - (static_cast<float>(j) + 0.5f) / static_cast<float>(env.height);
  direction_from_uv(u, v, out_direction);

  const float w_ij = env.pixel_weights[static_cast<size_t>(j * env.width + i)];
  out_pdf_solid_angle =
    w_ij * static_cast<float>(env.width * env.height) / (env.total_weight * 2.0f * PI * PI);
}

float ibl_pdf(const Environment& env, const vec3& direction) {
  if (env.total_weight <= 0.0f) {
    return 1.0f / (4.0f * PI);
  }
  float u = 0.0f;
  float v = 0.0f;
  uv_from_direction(direction, u, v);
  const int i = texel_i(u, env.width);
  const int j = texel_j(v, env.height);
  const float w_ij = env.pixel_weights[static_cast<size_t>(j * env.width + i)];
  return w_ij * static_cast<float>(env.width * env.height) / (env.total_weight * 2.0f * PI * PI);
}
