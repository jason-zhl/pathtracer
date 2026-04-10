#ifndef PLASTIC_H
#define PLASTIC_H

#include <algorithm>
#include <cmath>
#include "material.h"

class plastic : public material {
  public:
    explicit plastic(const color& albedo, double roughness)
      : albedo_(albedo), roughness_(roughness), alpha_(roughness * roughness), a2_(alpha_ * alpha_) {}

    bool scatter(const ray& r_in, const intersection& rec, color& attenuation, ray& scattered) const override {
      vec3 n = unit_vector(rec.surface->normal(rec.point));
      if (dot(r_in.direction(), n) > 0.0) {
        n = -n;
      }

      const vec3 wi = -unit_vector(r_in.direction());
      const double cos_i = std::max(0.0, dot(n, wi));

      if (random_double() * n_sum < n_spec) {
        for (int attempt = 0; attempt < k_max_spec_tries; ++attempt) {
          const vec3 h = sample_ggx(n);
          const double ndoth = dot(n, h);
          const double wih = dot(wi, h);
          if (ndoth <= k_eps || wih <= k_eps) {
            continue;
          }

          const vec3 wo = unit_vector(reflect(wi, h));
          const double ndotwo = dot(n, wo);
          if (ndotwo <= k_eps) {
            continue;
          }

          const color f = eval_brdf(wi, wo, n, cos_i);
          const double pdf_d = pdf_diffuse(ndotwo);
          const double pdf_s = pdf_specular(wi, wo, n);
          const double mis_pdf = (n_diff * pdf_d + n_spec * pdf_s) / n_sum;
          if (mis_pdf <= k_eps) {
            continue;
          }

          attenuation = f * ndotwo / mis_pdf;
          scattered = ray(rec.point + n * surface_offset, wo);
          return true;
        }
        return false;
      }

      const vec3 wo = unit_vector(lambertian_random(n));
      const double ndotwo = dot(n, wo);
      if (ndotwo <= k_eps) {
        return false;
      }

      const color f = eval_brdf(wi, wo, n, cos_i);
      const double pdf_d = pdf_diffuse(ndotwo);
      const double pdf_s = pdf_specular(wi, wo, n);
      const double mis_pdf = (n_diff * pdf_d + n_spec * pdf_s) / n_sum;
      if (mis_pdf <= k_eps) {
        return false;
      }

      attenuation = f * ndotwo / mis_pdf;
      scattered = ray(rec.point + n * surface_offset, wo);
      return true;
    }

    color eval(const ray& r_in, const intersection& rec, const vec3& wo) const override {
      vec3 n = unit_vector(rec.surface->normal(rec.point));
      if (dot(r_in.direction(), n) > 0.0) {
        n = -n;
      }
      const vec3 wi = -unit_vector(r_in.direction());
      const double cos_i = std::max(0.0, dot(n, wi));
      const vec3 wou = unit_vector(wo);
      return eval_brdf(wi, wou, n, cos_i);
    }

    double pdf(const ray& r_in, const intersection& rec, const vec3& wo) const override {
      vec3 n = unit_vector(rec.surface->normal(rec.point));
      if (dot(r_in.direction(), n) > 0.0) {
        n = -n;
      }
      const vec3 wi = -unit_vector(r_in.direction());
      const vec3 wou = unit_vector(wo);
      const double ndotwo = dot(n, wou);
      if (ndotwo <= k_eps) {
        return 0.0;
      }
      const double pdf_d = pdf_diffuse(ndotwo);
      const double pdf_s = pdf_specular(wi, wou, n);
      return (n_diff * pdf_d + n_spec * pdf_s) / n_sum;
    }

  private:
    color albedo_;
    double roughness_;
    double alpha_;
    double a2_;
    static constexpr double surface_offset = 1e-3;
    static constexpr double k_dielectric_f0 = 0.04;
    static constexpr double k_eps = 1e-8;
    static constexpr int k_max_spec_tries = 32;
    // Balance heuristic: n_diffuse = n_specular = 1 → pick each technique with probability 1/2.
    static constexpr double n_diff = 0.3;
    static constexpr double n_spec = 0.7;
    static constexpr double n_sum = n_diff + n_spec;

    static double pdf_diffuse(double ndotwo) { return ndotwo / PI; }

    double pdf_specular(const vec3& wi, const vec3& wo, const vec3& n) const {
      const double ndotwo = dot(n, wo);
      if (ndotwo <= 0.0) {
        return 0.0;
      }
      const vec3 h = unit_vector(wi + wo);
      const double ndoth = dot(n, h);
      const double wih = dot(wi, h);
      if (ndoth <= 0.0 || wih <= 0.0) {
        return 0.0;
      }
      const double D = ggx(h, n);
      return (D * ndoth) / (4.0 * std::abs(wih));
    }

    color eval_brdf(const vec3& wi, const vec3& wo, const vec3& n, double cos_i) const {
      const color f_diff = albedo_ / PI;
      const double ndotwo = dot(n, wo);
      double f_spec = 0.0;
      if (ndotwo > 0.0 && cos_i > 0.0) {
        const vec3 h = unit_vector(wi + wo);
        const double ndoth = std::max(0.0, dot(n, h));
        const double wih = std::max(0.0, dot(wi, h));
        if (ndoth > 0.0 && wih > 0.0) {
          const double D = ggx(h, n);
          const double F_micro = fresnel(wih);
          const double G = smith_g1(wi, n) * smith_g1(wo, n);
          f_spec = (D * G * F_micro) / (4.0 * cos_i * ndotwo);
        }
      }
      return f_diff + color(f_spec, f_spec, f_spec);
    }

    double fresnel(double cos_theta) const {
      cos_theta = std::clamp(cos_theta, -1.0, 1.0);
      const double t = 1.0 - std::fabs(cos_theta);
      return k_dielectric_f0 + (1.0 - k_dielectric_f0) * (t * t * t * t * t);
    }

    double ggx(const vec3& h, const vec3& n) const {
      const double c2 = std::pow(dot(h, n), 2);
      const double d = (c2 * (a2_ - 1.0) + 1.0);
      return a2_ / (PI * d * d);
    }

    vec3 sample_ggx(const vec3& n) const {
      const double xi1 = random_double();
      const double xi2 = random_double();
      const double cos_theta =
        std::sqrt(std::max(0.0, (1.0 - xi2) / (1.0 + (a2_ - 1.0) * xi2)));
      const double sin_theta = std::sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));
      const double phi = 2.0 * PI * xi1;
      vec3 t, b;
      orthonormal_basis(n, t, b);
      const double cos_p = std::cos(phi);
      const double sin_p = std::sin(phi);
      return unit_vector(t * (sin_theta * cos_p) + b * (sin_theta * sin_p) + n * cos_theta);
    }

    static vec3 reflect(const vec3& wi, const vec3& m) {
      return 2.0 * dot(wi, m) * m - wi;
    }

    double smith_g1(const vec3& v, const vec3& n) const {
      const double ndotv = dot(n, v);
      if (ndotv <= 0.0) {
        return 0.0;
      }
      return 2.0 * ndotv / (ndotv + std::sqrt(a2_ + (1.0 - a2_) * ndotv * ndotv));
    }
};

#endif
