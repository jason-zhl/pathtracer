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

            pixel_color += ray_colour(r, max_depth, scene);
          }
          pixel_color /= static_cast<double>(samples_per_pixel_);
          write_color(*out, pixel_color);
        }
      }
    }

  private:
    color ray_colour(const ray& r, int max_depth, const world& scene) const {
      if (max_depth <= 0)
        return color(0, 0, 0);

      intersection isect;
      if (scene.hit(r, isect)) {
        vec3 n = unit_vector(isect.surface->normal(isect.point));
        if (dot(r.direction(), n) > 0.0) {
          n = -n;
        }
        const vec3 diffuse_direction = unit_vector(lambertian_random(n));
        const double surface_offset = 1e-3;
        const vec3 scatter_origin = isect.point + n * surface_offset;
        // Temporary albedo of 0.5
        return 0.5 * ray_colour(ray(scatter_origin, diffuse_direction), max_depth - 1, scene);
      }

      vec3 unit_direction = unit_vector(r.direction());
      auto a = 0.5*(unit_direction.y() + 1.0);
      return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
    }

    int image_width_ = 0;
    int image_height_ = 0;
    int samples_per_pixel_ = 50;
    int max_depth = 10;
    vec3 center_;
    vec3 pixel00_;
    vec3 pixel_delta_horizontal_;
    vec3 pixel_delta_vertical_;
};

#endif
