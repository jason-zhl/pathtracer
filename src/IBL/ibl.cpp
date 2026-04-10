#include "IBL/ibl.h"

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

}  // namespace

void ibl::build_sampling_distribution() {
  pixel_weights_.assign(static_cast<size_t>(width_) * static_cast<size_t>(height_), 0.0);
  marginal_cdf_.assign(static_cast<size_t>(height_) + 1, 0.0);
  cond_cdf_.assign(static_cast<size_t>(height_) * (static_cast<size_t>(width_) + 1), 0.0);

  for (int j = 0; j < height_; ++j) {
    const double v_center = 1.0 - (static_cast<double>(j) + 0.5) / static_cast<double>(height_);
    const double theta = (v_center - 0.5) * PI;
    const double sin_theta = std::sin(theta);
    const double row_sin = std::max(sin_theta, 0.0);

    for (int i = 0; i < width_; ++i) {
      const int idx = (j * width_ + i) * 3;
      const double r = static_cast<double>(texture_[idx + 0]);
      const double g = static_cast<double>(texture_[idx + 1]);
      const double b = static_cast<double>(texture_[idx + 2]);
      const double lum = luminance_rgb(r, g, b);
      pixel_weights_[static_cast<size_t>(j * width_ + i)] = lum * row_sin + 1e-12;
    }

    const size_t row_off = static_cast<size_t>(j) * (static_cast<size_t>(width_) + 1);
    cond_cdf_[row_off] = 0.0;
    for (int i = 0; i < width_; ++i) {
      cond_cdf_[row_off + static_cast<size_t>(i) + 1] =
        cond_cdf_[row_off + static_cast<size_t>(i)] + pixel_weights_[static_cast<size_t>(j * width_ + i)];
    }

    const double row_sum = cond_cdf_[row_off + static_cast<size_t>(width_)];
    marginal_cdf_[static_cast<size_t>(j) + 1] = marginal_cdf_[static_cast<size_t>(j)] + row_sum;
  }

  total_weight_ = marginal_cdf_[static_cast<size_t>(height_)];
}

double ibl::luminance_rgb(double r, double g, double b) {
  return 0.212671 * r + 0.71516 * g + 0.072169 * b;
}

ibl::ibl(const std::string& file_name) : texture_(nullptr), width_(0), height_(0), channels_(0) {
  float* data = stbi_loadf(file_name.c_str(), &width_, &height_, &channels_, 3);
  if (!data) throw std::runtime_error(std::string("Failed to load IBL texture: ") + stbi_failure_reason());
  if (width_ < 1 || height_ < 1) {
    stbi_image_free(data);
    throw std::runtime_error("IBL texture has invalid dimensions");
  }
  texture_ = data;
  build_sampling_distribution();
}

ibl::ibl(ibl&& other) noexcept
    : texture_(other.texture_),
      width_(other.width_),
      height_(other.height_),
      channels_(other.channels_),
      marginal_cdf_(std::move(other.marginal_cdf_)),
      cond_cdf_(std::move(other.cond_cdf_)),
      pixel_weights_(std::move(other.pixel_weights_)),
      total_weight_(other.total_weight_) {
  other.texture_ = nullptr;
  other.width_ = other.height_ = other.channels_ = 0;
  other.total_weight_ = 0.0;
}

ibl& ibl::operator=(ibl&& other) noexcept {
  if (this != &other) {
    if (texture_) {
      stbi_image_free(texture_);
    }
    texture_ = other.texture_;
    width_ = other.width_;
    height_ = other.height_;
    channels_ = other.channels_;
    marginal_cdf_ = std::move(other.marginal_cdf_);
    cond_cdf_ = std::move(other.cond_cdf_);
    pixel_weights_ = std::move(other.pixel_weights_);
    total_weight_ = other.total_weight_;
    other.texture_ = nullptr;
    other.width_ = other.height_ = other.channels_ = 0;
    other.total_weight_ = 0.0;
  }
  return *this;
}

ibl::~ibl() {
  if (texture_) {
    stbi_image_free(texture_);
  }
}

void ibl::uv_from_direction(const vec3& d, double& u, double& v) const {
  const vec3 du = unit_vector(d);
  const double phi = std::atan2(du.z(), du.x());
  const double theta = std::asin(std::clamp(du.y(), -1.0, 1.0));
  u = phi * INV_2PI;
  u = u - std::floor(u);
  v = theta * INV_PI + 0.5;
}

void ibl::direction_from_uv(double u, double v, vec3& out) const {
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

vec3 ibl::value(const vec3& direction) const {
  const vec3 d = unit_vector(direction);
  const double phi = std::atan2(d.z(), d.x());
  const double theta = std::asin(std::clamp(d.y(), -1.0, 1.0));

  const double u = phi * INV_2PI;
  const double v = theta * INV_PI + 0.5;

  return sample_equirectangular(u, v);
}

vec3 ibl::sample_equirectangular(double u, double v) const {
  double uu = std::fmod(u, 1.0);
  if (uu < 0.0) {
    uu += 1.0;
  }
  const double vv = std::clamp(v, 0.0, 1.0);
  return bilinear_interpolation(uu, vv);
}

vec3 ibl::bilinear_interpolation(double u, double v) const {
  const double fx = u * static_cast<double>(width_ - 1);
  const double fy = (1.0 - v) * static_cast<double>(height_ - 1);

  const int i0 = static_cast<int>(fx);
  const int j0 = static_cast<int>(fy);
  const int i1 = std::min(i0 + 1, width_ - 1);
  const int j1 = std::min(j0 + 1, height_ - 1);

  const double tx = fx - static_cast<double>(i0);
  const double ty = fy - static_cast<double>(j0);

  auto texel = [this](int i, int j) -> vec3 {
    const int idx = (j * width_ + i) * 3;
    return vec3(static_cast<double>(texture_[idx + 0]), static_cast<double>(texture_[idx + 1]),
                static_cast<double>(texture_[idx + 2]));
  };

  const vec3 c00 = texel(i0, j0);
  const vec3 c10 = texel(i1, j0);
  const vec3 c01 = texel(i0, j1);
  const vec3 c11 = texel(i1, j1);

  const vec3 c0 = c00 * (1.0 - tx) + c10 * tx;
  const vec3 c1 = c01 * (1.0 - tx) + c11 * tx;
  return c0 * (1.0 - ty) + c1 * ty;
}

void ibl::sample_direction(vec3& out_direction, double& out_pdf_solid_angle) const {
  if (total_weight_ <= 0.0) {
    out_pdf_solid_angle = 1.0 / (4.0 * PI);
    for (;;) {
      const vec3 p(random_double() * 2.0 - 1.0, random_double() * 2.0 - 1.0, random_double() * 2.0 - 1.0);
      const double len2 = p.length_squared();
      if (len2 <= 1.0 && len2 > 1e-20) {
        out_direction = unit_vector(p);
        break;
      }
    }
    return;
  }

  const double r1 = random_double() * total_weight_;
  const auto row_it = std::upper_bound(marginal_cdf_.begin(), marginal_cdf_.end(), r1);
  int j = static_cast<int>(row_it - marginal_cdf_.begin()) - 1;
  j = std::clamp(j, 0, height_ - 1);

  const double row_lo = marginal_cdf_[static_cast<size_t>(j)];
  const double row_hi = marginal_cdf_[static_cast<size_t>(j) + 1];
  const double row_sum = row_hi - row_lo;
  const double r2 = random_double() * row_sum;

  const size_t row_off = static_cast<size_t>(j) * (static_cast<size_t>(width_) + 1);
  const auto col_it = std::upper_bound(cond_cdf_.begin() + static_cast<std::ptrdiff_t>(row_off),
    cond_cdf_.begin() + static_cast<std::ptrdiff_t>(row_off + static_cast<size_t>(width_) + 1), r2);
  int i = static_cast<int>(col_it - cond_cdf_.begin() - static_cast<std::ptrdiff_t>(row_off)) - 1;
  i = std::clamp(i, 0, width_ - 1);

  const double u = (static_cast<double>(i) + 0.5) / static_cast<double>(width_);
  const double v = 1.0 - (static_cast<double>(j) + 0.5) / static_cast<double>(height_);
  direction_from_uv(u, v, out_direction);

  const double w_ij = pixel_weights_[static_cast<size_t>(j * width_ + i)];
  out_pdf_solid_angle = w_ij * static_cast<double>(width_ * height_) / (total_weight_ * 2.0 * PI * PI);
}

double ibl::pdf(const vec3& direction) const {
  if (total_weight_ <= 0.0) {
    return 1.0 / (4.0 * PI);
  }
  double u = 0.0;
  double v = 0.0;
  uv_from_direction(direction, u, v);
  const int i = texel_i(u, width_);
  const int j = texel_j(v, height_);
  const double w_ij = pixel_weights_[static_cast<size_t>(j * width_ + i)];
  return w_ij * static_cast<double>(width_ * height_) / (total_weight_ * 2.0 * PI * PI);
}
