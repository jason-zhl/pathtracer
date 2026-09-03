#ifndef CAMERA_H
#define CAMERA_H

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "global.h"
#include "cuda/ray_colour.h"
#include "host_device.h"
#include "timer.h"
#include "world.h"

class camera {
  public:
    camera(int image_width, float aspect_ratio, int samples_per_pixel,
           const vec3& look_from, const vec3& look_at, const vec3& up,
           float vfov_degrees, float focus_dist = 5.5f, float aperture_radius = 0.0f,
           float exposure = 1.0f)
      : image_width_(image_width), samples_per_pixel_(samples_per_pixel),
        aperture_radius_(aperture_radius), exposure_(exposure) {
      image_height_ = static_cast<int>(image_width_ / aspect_ratio);
      if (image_height_ < 1) {
        image_height_ = 1;
      }

      center_ = look_from;

      const float theta = vfov_degrees * (PI / 180.0f);
      const float viewport_height = 2.0f * tanf(theta * 0.5f) * focus_dist;
      const float viewport_width = viewport_height
        * (static_cast<float>(image_width_) / static_cast<float>(image_height_));

      const vec3 w = unit_vector(look_at - look_from);
      const vec3 u = unit_vector(cross(up, w));
      const vec3 v = cross(w, u);

      const vec3 viewport_horizontal = viewport_width * u;
      const vec3 viewport_vertical = viewport_height * v;

      pixel_delta_horizontal_ = viewport_horizontal / static_cast<float>(image_width_);
      pixel_delta_vertical_ = viewport_vertical / static_cast<float>(image_height_);

      const vec3 viewport_upper_left = center_ + w * focus_dist
        - viewport_horizontal * 0.5f + viewport_vertical * 0.5f;
      pixel00_ = viewport_upper_left + 0.5f * (pixel_delta_horizontal_ + pixel_delta_vertical_);

      defocus_disk_u_ = aperture_radius_ * u;
      defocus_disk_v_ = aperture_radius_ * v;
    }

    explicit camera(const std::string& config_path);

    HOST_DEVICE ray primary_ray(int i, int j, RNG& rng) const {
      const float u = rng.next(-0.5f, 0.5f);
      const float v = rng.next(-0.5f, 0.5f);
      const vec3 pixel_center = pixel00_ + (static_cast<float>(i) * pixel_delta_horizontal_)
        - (static_cast<float>(j) * pixel_delta_vertical_);
      const vec3 sample_point = pixel_center + u * pixel_delta_horizontal_
        - v * pixel_delta_vertical_;

      vec3 origin = center_;
      if (aperture_radius_ > 0.0f) {
        const vec3 disk = random_in_unit_disk(rng);
        origin += disk.x() * defocus_disk_u_ + disk.y() * defocus_disk_v_;
      }
      return ray(origin, sample_point - origin);
    }

    HOST_DEVICE int image_width() const { return image_width_; }
    HOST_DEVICE int image_height() const { return image_height_; }
    HOST_DEVICE int samples_per_pixel() const { return samples_per_pixel_; }
    HOST_DEVICE int max_depth() const { return max_depth_; }
    HOST_DEVICE const interval& ray_t() const { return ray_t_; }
    HOST_DEVICE color tonemap(const color& hdr) const {
      return gamma_filter(ACESFilm(hdr * exposure_));
    }

    void render(const World& world, std::ofstream* out) const {
      if (out == nullptr) {
        return;
      }

      *out << "P3\n" << image_width_ << " " << image_height_ << "\n255\n";

      const auto total_pixels = static_cast<int64_t>(image_width_) * image_height_;
      std::vector<color> pixels(static_cast<std::size_t>(total_pixels));

      render_gpu(world, pixels.data(), total_pixels);
      // render_cpu(world, pixels.data(), total_pixels);

      for (int64_t p = 0; p < total_pixels; ++p) {
        write_color(*out, pixels[static_cast<std::size_t>(p)]);
      }
    }

  private:
    void render_cpu(const World& world, color* pixels, int64_t total_pixels) const;

    void render_gpu(const World& world, color* pixels, int64_t total_pixels) const {
      if (pixels == nullptr || total_pixels <= 0) {
        return;
      }

      timer render_timer(total_pixels);
      if (!cuda_ray_colour(pixels, total_pixels, *this, world.view())) {
        return;
      }
      render_timer.print_complete("Render complete");
    }

    int image_width_ = 0;
    int image_height_ = 0;
    int samples_per_pixel_ = 50;
    int max_depth_ = 5;
    const interval ray_t_{1e-3f, INF};
    vec3 center_;
    vec3 pixel00_;
    vec3 pixel_delta_horizontal_;
    vec3 pixel_delta_vertical_;
    float aperture_radius_ = 0.0f;
    vec3 defocus_disk_u_;
    vec3 defocus_disk_v_;
    float exposure_ = 1.0f;
};

#include "trace.h"

inline void camera::render_cpu(const World& world, color* pixels, int64_t total_pixels) const {
  if (pixels == nullptr || total_pixels <= 0) {
    return;
  }

  timer render_timer(total_pixels);
  const Scene scene = world.view();

  for (auto j{0}; j < image_height_; j++) {
    for (auto i{0}; i < image_width_; i++) {
      color pixel_color(0, 0, 0);
      for (auto s{0}; s < samples_per_pixel_; s++) {
        RNG rng = make_rng(i, j, s);
        pixel_color += ray_colour(primary_ray(i, j, rng), *this, scene, rng);
      }
      pixel_color /= static_cast<float>(samples_per_pixel_);
      pixels[static_cast<int64_t>(j) * image_width_ + i] = tonemap(pixel_color);
    }

    const auto done = static_cast<int64_t>(j + 1) * image_width_;
    render_timer.update_progress(done);
  }

  render_timer.print_complete("Render complete");
}

#endif
