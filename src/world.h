#ifndef WORLD_H
#define WORLD_H

#include "geometry.h"
#include <vector>
#include "IBL/ibl.h"

class world {
  public:
    world() = default;

    void add(shared_ptr<geometry> object) { objects.push_back(object); }

    void clear() { objects.clear(); }

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

  private:
    std::vector<shared_ptr<geometry>> objects;
    std::unique_ptr<ibl> ibl_;
};

#endif
