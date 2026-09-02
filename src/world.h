#ifndef WORLD_H
#define WORLD_H

#include "environment/environment.h"
#include "environment/solid.h"
#include "geometry/geometry.h"
#include "material/material.h"
#include <cmath>
#include <utility>
#include <vector>

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
    world() : env_(std::make_unique<solid>(vec3(1.0, 1.0, 1.0))) {}

    int add_material(const Material& mat) {
      materials_.push_back(mat);
      return static_cast<int>(materials_.size()) - 1;
    }

    bool has_material(int id) const {
      return id >= 0 && static_cast<std::size_t>(id) < materials_.size();
    }

    const Material& material(int id) const {
      return materials_.at(static_cast<std::size_t>(id));
    }

    int add(const Geometry& object) {
      geometries_.push_back(object);
      return static_cast<int>(geometries_.size()) - 1;
    }

    bool has_geometry(int id) const {
      return id >= 0 && static_cast<std::size_t>(id) < geometries_.size();
    }

    const Geometry& geometry(int id) const {
      return geometries_.at(static_cast<std::size_t>(id));
    }

    /** Register geometry already in `add` for next-event estimation. */
    void add_area_light(int geom_id) {
      if (has_geometry(geom_id)) {
        area_lights_.push_back(geom_id);
      }
    }

    bool has_area_lights() const { return !area_lights_.empty(); }

    const std::vector<Geometry>& geometries() const { return geometries_; }
    const std::vector<int>& area_light_ids() const { return area_lights_; }

    void clear() {
      geometries_.clear();
      area_lights_.clear();
      materials_.clear();
    }

    bool hit(const ray& r, const interval* t_range, intersection& isect) const {
      if (t_range == nullptr) {
        return false;
      }

      intersection closest;
      double closest_t = t_range->max;
      bool hit_anything = false;

      for (int i = 0; i < static_cast<int>(geometries_.size()); ++i) {
        intersection temp;
        if (geometries_[static_cast<std::size_t>(i)].hit(r, t_range, temp)) {
          if (!hit_anything || temp.t < closest_t) {
            temp.geom_id = i;
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

    void set_environment(std::unique_ptr<environment> env) {
      if (env) {
        env_ = std::move(env);
      }
    }

    vec3 get_env(const vec3& direction) const { return env_->value(direction); }

    void sample_env(vec3& out_direction, double& out_pdf, RNG& rng) const {
      env_->sample_direction(out_direction, out_pdf, rng);
    }

    double env_pdf(const vec3& direction) const { return env_->pdf(direction); }

    /**
     * One-sample area direct lighting: uniform light, uniform point (area pdf), shadow ray.
     * MIS (power, β=2) vs mixture BSDF pdf at ω toward the sample reduces glossy double-count / spikes.
     */
    color area_light_nee(const ray& r_in, const intersection& isect, const vec3& n_shade,
      RNG& rng) const {
      if (area_lights_.empty() || !has_material(isect.mat_id)) {
        return color(0, 0, 0);
      }

      const auto n_lights = area_lights_.size();
      const auto idx = static_cast<size_t>(rng.next() * static_cast<double>(n_lights));
      const size_t pick = idx >= n_lights ? n_lights - 1 : idx;
      const int geom_id = area_lights_[pick];
      if (!has_geometry(geom_id)) {
        return color(0, 0, 0);
      }

      const Geometry& geom = geometry(geom_id);
      if (!has_material(geom.mat_id)) {
        return color(0, 0, 0);
      }

      vec3 pL;
      vec3 nL;
      double pdf_a = 0.0;
      if (!geom.sample_emitter_point(pL, nL, pdf_a, rng) || pdf_a <= 0.0) {
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
      light_isect.normal = nL;
      light_isect.mat_id = geom.mat_id;
      light_isect.geom_id = geom_id;

      const ray toward_light(isect.point, wo);
      const color Le = material(geom.mat_id).emitted(toward_light, light_isect);
      const color f = material(isect.mat_id).eval(r_in, isect, wo);

      const double pdf_nee =
        (pdf_a / static_cast<double>(n_lights)) * dist2 / std::max(cos_light, 1e-20);
      const double pdf_mat = material(isect.mat_id).pdf(r_in, isect, wo);
      const double mis_w = nee_mis_weight(pdf_nee, pdf_mat);

      return mis_w * f * Le * (cos_sh / std::max(pdf_nee, 1e-30));
    }

    /**
     * Solid-angle pdf at `shading_point` for “uniform light + uniform area point” (same Jacobian as NEE),
     * when the path direction `wo` hits `light_geom_id` at `light_point`. Zero if not a registered area light.
     */
    double area_light_pdf_nee_at_receiver(const vec3& shading_point, const vec3& wo_toward_light,
      int light_geom_id, const vec3& light_point) const {
      if (area_lights_.empty() || !has_geometry(light_geom_id)) {
        return 0.0;
      }
      bool registered = false;
      for (const int id : area_lights_) {
        if (id == light_geom_id) {
          registered = true;
          break;
        }
      }
      if (!registered) {
        return 0.0;
      }

      const Geometry& light_geom = geometry(light_geom_id);
      const vec3 wo = unit_vector(wo_toward_light);
      const vec3 nL = unit_vector(light_geom.normal(light_point));
      const double cos_light = dot(nL, -wo);
      if (cos_light <= 1e-20) {
        return 0.0;
      }

      const vec3 delta = light_point - shading_point;
      const double dist2 = delta.length_squared();
      if (dist2 < 1e-20) {
        return 0.0;
      }

      const double A = light_geom.surface_area();
      if (A <= 0.0) {
        return 0.0;
      }
      const double pdf_a = 1.0 / A;
      const auto n_lights = area_lights_.size();
      return (pdf_a / static_cast<double>(n_lights)) * dist2 / cos_light;
    }

  private:
    std::vector<Geometry> geometries_;
    std::vector<int> area_lights_;
    std::vector<Material> materials_;
    std::unique_ptr<environment> env_;
};

#endif
