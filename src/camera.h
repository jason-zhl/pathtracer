#ifndef CAMERA_H
#define CAMERA_H

#include <fstream>
#include "world.h"

class camera {
  public:
    camera(int image_width, double aspect_ratio, int samples_per_pixel = 10)
      : image_width_(image_width), samples_per_pixel_(samples_per_pixel) {
      image_height_ = static_cast<int>(image_width_ / aspect_ratio);

      const auto viewport_height = 9.0;
      const auto viewport_width = viewport_height * (double(image_width_) / image_height_);
      const auto focal_length = 4.5;

      center_ = vec3(0, 2, 0);

      const auto viewport_horizontal = vec3(viewport_width, 0, 0);
      const auto viewport_vertical = vec3(0, viewport_height, 0);

      pixel_delta_horizontal_ = viewport_horizontal / image_width_;
      pixel_delta_vertical_ = viewport_vertical / image_height_;

      const auto viewport_upper_left = center_ + vec3(0, 0, focal_length)
        - viewport_horizontal / 2 + viewport_vertical / 2;
      pixel00_ = viewport_upper_left + 0.5 * (pixel_delta_horizontal_ + pixel_delta_vertical_);
    }

    int image_width() const { return image_width_; }
    int image_height() const { return image_height_; }

    void render(const world& scene, std::ofstream* out) const {
      if (out == nullptr) {
        return;
      }

      *out << "P3\n" << image_width_ << " " << image_height_ << "\n255\n";

      for (auto j{0}; j < image_height_; j++) {
        for (auto i{0}; i < image_width_; i++) {
          const auto pixel_center = pixel00_ + (i * pixel_delta_horizontal_) - (j * pixel_delta_vertical_);

          color pixel_color(0, 0, 0);
          for (auto s{0}; s < samples_per_pixel_; s++) {
            const auto u = random_double(-0.5, 0.5);
            const auto v = random_double(-0.5, 0.5);
            const auto sample_point = pixel_center
              + u * pixel_delta_horizontal_
              - v * pixel_delta_vertical_;
            const ray r(center_, sample_point - center_);

            intersection isect;
            if (scene.hit(r, isect)) {
              const vec3 n = unit_vector(isect.surface->normal(isect.point));
              pixel_color += 0.5 * color(n.x() + 1, n.y() + 1, n.z() + 1);
            } else {
              pixel_color += ray_colour(r);
            }
          }
          pixel_color /= static_cast<double>(samples_per_pixel_);
          write_color(*out, pixel_color);
        }
      }
    }

  private:
    color ray_colour(const ray& r) const {
      const vec3 unit_direction = unit_vector(r.direction());
      const auto a = 0.5 * (unit_direction.y() + 1.0);
      return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
    }

    int image_width_ = 0;
    int image_height_ = 0;
    int samples_per_pixel_ = 10;
    vec3 center_;
    vec3 pixel00_;
    vec3 pixel_delta_horizontal_;
    vec3 pixel_delta_vertical_;
};

#endif
