#ifndef WORLD_H
#define WORLD_H

#include "geometry.h"
#include "material/diffuse_light.h"
#include <cmath>
#include <utility>
#include <vector>
#include "IBL/ibl.h"

/** β = 2 power heuristic (same as camera / Veach). */
inline double nee_mis_weight(double pdf_nee, double pdf_mat) {
  constexpr double beta = 2.0;
  const double a = std::pow(pdf_nee, beta);
  const double b = std::pow(pdf_mat, beta);
  const double d = a + b;
  return d > 0.0 ? a / d : 0.0;
}

class world {
  public:
    world() = default;

    void add(shared_ptr<geometry> object) { objects.push_back(object); }

    /** Register geometry that uses `em` for next-event estimation (same pointer should already be in `add`). */
    void add_area_light(shared_ptr<geometry> object, shared_ptr<diffuse_light> em) {
      area_lights_.emplace_back(std::move(object), std::move(em));
    }

    bool has_area_lights() const { return !area_lights_.empty(); }

    void clear() {
      objects.clear();
      area_lights_.clear();
    }

    bool hit(const ray& r, const interval* t_range, intersection& isect) const {
      if (t_range == nullptr) {
        return false;
      }

      intersection closest;
      double closest_t = t_range->max;
      bool hit_anything = false;

      for (const auto& obj : objects) {
        intersection temp;
        if (obj->hit(r, t_range, temp)) {
          if (!hit_anything || temp.t < closest_t) {
            closest = temp;
            closest_t = temp.t;
            hit_anything = true;
          }
        }
      }

      if (hit_anything) {
        isect = closest;
      }
      return hit_anything;
    }

    void set_ibl(std::unique_ptr<ibl> env) { ibl_ = std::move(env); }

    bool has_ibl() const { return ibl_ != nullptr; }

    vec3 get_env(const vec3& direction) const {
      if (ibl_) {
        return ibl_->value(direction);
      }
      return vec3(1.0, 1.0, 1.0);
    }

    void sample_env(vec3& out_direction, double& out_pdf) const {
      if (ibl_) {
        ibl_->sample_direction(out_direction, out_pdf);
      } else {
        out_pdf = 0.0;
        out_direction = vec3(0, 1, 0);
      }
    }

    double ibl_pdf(const vec3& direction) const {
      if (ibl_) {
        return ibl_->pdf(direction);
      }
      return 0.0;
    }

    /**
     * One-sample area direct lighting: uniform light, uniform point (area pdf), shadow ray.
     * MIS (power, β=2) vs mixture BSDF pdf at ω toward the sample reduces glossy double-count / spikes.
     */
    color area_light_nee(const ray& r_in, const intersection& isect, const vec3& n_shade) const {
      if (area_lights_.empty() || isect.mat == nullptr) {
        return color(0, 0, 0);
      }

      const auto n_lights = area_lights_.size();
      const auto idx = static_cast<size_t>(random_double() * static_cast<double>(n_lights));
      const size_t pick = idx >= n_lights ? n_lights - 1 : idx;

      const shared_ptr<geometry>& geom = area_lights_[pick].first;
      const shared_ptr<diffuse_light>& em = area_lights_[pick].second;

      vec3 pL;
      vec3 nL;
      double pdf_a = 0.0;
      if (!geom->sample_emitter_point(pL, nL, pdf_a) || pdf_a <= 0.0) {
        return color(0, 0, 0);
      }

      const vec3 d = pL - isect.point;
      const double dist2 = d.length_squared();
      if (dist2 < 1e-20) {
        return color(0, 0, 0);
      }
      const double dist = std::sqrt(dist2);
      const vec3 wo = d / dist;

      const double cos_sh = dot(n_shade, wo);
      if (cos_sh <= 0.0) {
        return color(0, 0, 0);
      }

      const double cos_light = dot(nL, -wo);
      if (cos_light <= 0.0) {
        return color(0, 0, 0);
      }

      const interval shadow_range(1e-3, dist - 1e-3);
      if (shadow_range.min >= shadow_range.max) {
        return color(0, 0, 0);
      }

      const ray shadow_ray(isect.point + n_shade * 1e-3, wo);
      intersection occ;
      if (hit(shadow_ray, &shadow_range, occ)) {
        return color(0, 0, 0);
      }

      intersection light_isect;
      light_isect.point = pL;
      light_isect.surface = geom.get();
      light_isect.mat = em;

      const ray toward_light(isect.point, wo);
      const color Le = em->emitted(toward_light, light_isect);
      const color f = isect.mat->eval(r_in, isect, wo);

      const double pdf_nee =
        (pdf_a / static_cast<double>(n_lights)) * dist2 / std::max(cos_light, 1e-20);
      const double pdf_mat = isect.mat->pdf(r_in, isect, wo);
      const double mis_w = nee_mis_weight(pdf_nee, pdf_mat);

      return mis_w * f * Le * (cos_sh / std::max(pdf_nee, 1e-30));
    }

    /**
     * Solid-angle pdf at `shading_point` for “uniform light + uniform area point” (same Jacobian as NEE),
     * when the path direction `wo` hits `light_geom` at `light_point`. Zero if `light_geom` is not registered.
     */
    double area_light_pdf_nee_at_receiver(const vec3& shading_point, const vec3& wo_toward_light,
      const geometry* light_geom, const vec3& light_point) const {
      if (area_lights_.empty() || light_geom == nullptr) {
        return 0.0;
      }
      bool registered = false;
      for (const auto& pr : area_lights_) {
        if (pr.first.get() == light_geom) {
          registered = true;
          break;
        }
      }
      if (!registered) {
        return 0.0;
      }

      const vec3 wo = unit_vector(wo_toward_light);
      const vec3 nL = unit_vector(light_geom->normal(light_point));
      const double cos_light = dot(nL, -wo);
      if (cos_light <= 1e-20) {
        return 0.0;
      }

      const vec3 delta = light_point - shading_point;
      const double dist2 = delta.length_squared();
      if (dist2 < 1e-20) {
        return 0.0;
      }

      const double A = light_geom->surface_area();
      if (A <= 0.0) {
        return 0.0;
      }
      const double pdf_a = 1.0 / A;
      const auto n_lights = area_lights_.size();
      return (pdf_a / static_cast<double>(n_lights)) * dist2 / cos_light;
    }

  private:
    std::vector<shared_ptr<geometry>> objects;
    std::vector<std::pair<shared_ptr<geometry>, shared_ptr<diffuse_light>>> area_lights_;
    std::unique_ptr<ibl> ibl_;
};

#endif
