#ifndef WORLD_H
#define WORLD_H

#include "geometry.h"
#include <vector>

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

  private:
    std::vector<shared_ptr<geometry>> objects;
};

#endif
