#ifndef CAMERA_H
#define CAMERA_H

#include <cstdint>
#include <fstream>
#include <vector>

#include "cuda/ray_colour.h"
#include "host_device.h"
#include "timer.h"
#include "world.h"

class camera {
  public:
    camera(int image_width, double aspect_ratio, int samples_per_pixel = 10)
      : image_width_(image_width), samples_per_pixel_(samples_per_pixel) {
      image_height_ = static_cast<int>(image_width_ / aspect_ratio);

      const auto viewport_height = 4.0;
      const auto viewport_width = viewport_height * (double(image_width_) / image_height_);
      const auto focal_length = 5.5;

      center_ = vec3(0, 3, 0);

      const auto viewport_horizontal = vec3(viewport_width, 0, 0);
      const auto viewport_vertical = vec3(0, viewport_height, 0);

      pixel_delta_horizontal_ = viewport_horizontal / image_width_;
      pixel_delta_vertical_ = viewport_vertical / image_height_;

      const auto viewport_upper_left = center_ + vec3(0, 0, focal_length)
        - viewport_horizontal / 2 + viewport_vertical / 2;
      pixel00_ = viewport_upper_left + 0.5 * (pixel_delta_horizontal_ + pixel_delta_vertical_);
    }

    HOST_DEVICE ray primary_ray(int i, int j, double u, double v) const {
      const vec3 pixel_center = pixel00_ + (static_cast<double>(i) * pixel_delta_horizontal_)
        - (static_cast<double>(j) * pixel_delta_vertical_);
      const vec3 sample_point = pixel_center + u * pixel_delta_horizontal_
        - v * pixel_delta_vertical_;
      return ray(center_, sample_point - center_);
    }

    HOST_DEVICE int image_width() const { return image_width_; }
    HOST_DEVICE int image_height() const { return image_height_; }
    HOST_DEVICE int samples_per_pixel() const { return samples_per_pixel_; }
    HOST_DEVICE int max_depth() const { return max_depth_; }
    HOST_DEVICE const interval& ray_t() const { return ray_t_; }

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
    const interval ray_t_{1e-3, INF};
    vec3 center_;
    vec3 pixel00_;
    vec3 pixel_delta_horizontal_;
    vec3 pixel_delta_vertical_;
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
        const auto u = rng.next(-0.5, 0.5);
        const auto v = rng.next(-0.5, 0.5);
        pixel_color += ray_colour(primary_ray(i, j, u, v), *this, scene, rng);
      }
      pixel_color /= static_cast<double>(samples_per_pixel_);
      pixels[static_cast<int64_t>(j) * image_width_ + i] = pixel_color;
    }

    const auto done = static_cast<int64_t>(j + 1) * image_width_;
    render_timer.update_progress(done);
  }

  render_timer.print_complete("Render complete");
}

#endif
