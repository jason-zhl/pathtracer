#ifndef CAMERA_H
#define CAMERA_H

#include <cmath>
#include <cstdint>
#include <fstream>
#include <memory>
#include "timer.h"
#include "material/diffuse_light.h"
#include "material/material.h"
#include "world.h"

class camera {
  public:
    camera(int image_width, double aspect_ratio, int samples_per_pixel = 10)
      : image_width_(image_width), samples_per_pixel_(samples_per_pixel) {
      image_height_ = static_cast<int>(image_width_ / aspect_ratio);

      const auto viewport_height = 4.0;
      const auto viewport_width = viewport_height * (double(image_width_) / image_height_);
      const auto focal_length = 2.5;

      center_ = vec3(0, 3, 0);

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

      const auto total_pixels = static_cast<int64_t>(image_width_) * image_height_;
      timer render_timer(total_pixels);

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

        const auto done = static_cast<int64_t>(j + 1) * image_width_;
        render_timer.update_progress(done);
      }

      render_timer.print_complete("Render complete");
    }

  private:
    /** Power heuristic MIS weight for strategy with pdf_self vs the other; β = 2 (Veach). */
    static double mis_weight_power(double pdf_self, double pdf_other) {
      constexpr double beta = 2.0;
      const double a = std::pow(pdf_self, beta);
      const double b = std::pow(pdf_other, beta);
      const double denom = a + b;
      return denom > 0.0 ? a / denom : 0.0;
    }

    color ray_colour(const ray& r, int max_depth, const world& scene,
      const intersection* prev_isect = nullptr, const ray* prev_ray_in = nullptr) const {
      if (max_depth <= 0)
        return color(0, 0, 0);

      intersection isect;
      if (!scene.hit(r, &ray_t_, isect)) {
        return scene.get_env(r.direction());
      }

      vec3 n = unit_vector(isect.surface->normal(isect.point));
      if (dot(r.direction(), n) > 0.0) {
        n = -n;
      }

      color L = color(0, 0, 0);
      if (const auto emissive = std::dynamic_pointer_cast<diffuse_light>(isect.mat)) {
        color Le = emissive->emitted(r, isect);
        if (prev_isect != nullptr && prev_ray_in != nullptr && scene.has_area_lights()) {
          const vec3 wo = unit_vector(r.direction());
          const double pdf_nee =
            scene.area_light_pdf_nee_at_receiver(prev_isect->point, wo, isect.surface, isect.point);
          if (pdf_nee > 0.0 && prev_isect->mat != nullptr) {
            const double pdf_mat = prev_isect->mat->pdf(*prev_ray_in, *prev_isect, wo);
            const double w_bsdf = nee_mis_weight(pdf_mat, pdf_nee);
            Le *= w_bsdf;
          }
        }
        L += Le;
      }

      if (scene.has_ibl() && isect.mat != nullptr) {
        vec3 wo_env;
        double pdf_env = 0.0;
        scene.sample_env(wo_env, pdf_env);
        if (pdf_env > 0.0 && dot(n, wo_env) > 0.0) {
          ray env_ray(isect.point + n * 1e-3, wo_env);
          intersection shadow_isect;
          if (!scene.hit(env_ray, &ray_t_, shadow_isect)) {
            const color f_env = isect.mat->eval(r, isect, wo_env);
            const double pdf_mat = isect.mat->pdf(r, isect, wo_env);
            const double mis_w = mis_weight_power(pdf_env, pdf_mat);
            const vec3 Le = scene.get_env(wo_env);
            L += mis_w * f_env * Le * dot(n, wo_env) / pdf_env;
          }
        }
      }

      if (scene.has_area_lights() && isect.mat != nullptr
        && std::dynamic_pointer_cast<diffuse_light>(isect.mat) == nullptr) {
        L += scene.area_light_nee(r, isect, n);
      }

      ray scattered;
      color attenuation;
      if (isect.mat != nullptr && isect.mat->scatter(r, isect, attenuation, scattered)) {
        intersection bounce_isect;
        if (!scene.hit(scattered, &ray_t_, bounce_isect)) {
          const vec3 Le = scene.get_env(scattered.direction());
          if (scene.has_ibl()) {
            const double pdf_env = scene.ibl_pdf(scattered.direction());
            const double pdf_mat = isect.mat->pdf(r, isect, scattered.direction());
            const double mis_w = mis_weight_power(pdf_mat, pdf_env);
            L += mis_w * attenuation * Le;
          } else {
            L += attenuation * Le;
          }
        } else {
          L += attenuation * ray_colour(scattered, max_depth - 1, scene, &isect, &r);
        }
      }
      return L;
    }

    int image_width_ = 0;
    int image_height_ = 0;
    int samples_per_pixel_ = 50;
    int max_depth = 5;
    const interval ray_t_{1e-3, INF};
    vec3 center_;
    vec3 pixel00_;
    vec3 pixel_delta_horizontal_;
    vec3 pixel_delta_vertical_;
};

#endif
