#ifndef WORLD_H
#define WORLD_H

#include "geometry.h"
#include <limits>
#include <memory>
#include <vector>

class ray;

class world {
  public:
    world() = default;

    void add(std::shared_ptr<geometry> object) { objects.push_back(object); }

    void clear() { objects.clear(); }

    bool hit(const ray& r, intersection& isect) const {
      intersection closest;
      closest.t = std::numeric_limits<double>::infinity();
      bool hit_anything = false;

      for (const auto& obj : objects) {
        intersection temp;
        if (obj->hit(r, temp) && temp.t < closest.t) {
          closest = temp;
          hit_anything = true;
        }
      }

      if (hit_anything) {
        isect = closest;
      }
      return hit_anything;
    }

  private:
    std::vector<std::shared_ptr<geometry>> objects;
};

#endif
